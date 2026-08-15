#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/text_layout.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include <algorithm>
#include <cmath>
#include <optional>
#include <unordered_map>
#include <vector>

namespace p5
{
    namespace
    {
        FT_Library& freeTypeLibrary()
        {
            static FT_Library library = [] {
                FT_Library lib = nullptr;
                if (FT_Init_FreeType(&lib) != 0) {
                    error("Font: FT_Init_FreeType() failed");
                    return static_cast<FT_Library>(nullptr);
                }
                return lib;
            }();
            return library;
        }

        // Glyphs are rasterized once at a fixed source resolution (independent of any requested
        // textSize()) and converted into a signed distance field; a single atlas cell per glyph is then
        // reused for every display size via the SDF shader's smoothstep antialiasing.
        //
        // The source resolution (texels/em, since kDownsampleFactor is fixed at 1 below) needs to stay
        // high enough that small/thin glyph features (e.g. a lowercase 'a' bowl, or the ~2-texel-thick
        // crossbar of a lowercase 'e') still get several texels of SDF gradient across their width —
        // too low a density collapses most of a cell's values into a narrow, low-contrast band around
        // the 128 edge threshold, which then thresholds into visible noise/gaps once bilinear-magnified
        // on screen. Tried 48/4 = 12 texels/em first (nowhere near enough for small glyphs generally),
        // then 64/2 = 32 texels/em (fixed whole-glyph corruption but individual thin strokes like the
        // 'e' crossbar were still only ~2 texels thick and broke up); downsampling is dropped entirely
        // (factor 1) so the atlas keeps the full source raster's detail instead of averaging thin
        // features away.
        //
        // SDF crispness is not scale-invariant beyond this resolution, though: the encoded spread band
        // (kSpreadAtlasTexels) is a fixed number of *texels*, so displaying a glyph much larger than
        // the source resolution stretches that band into a visibly soft transition on screen. The
        // default here (128 texels/em) keeps text crisp up to a few hundred px before that becomes
        // noticeable; loadFont()'s sdfSourceEmPixels parameter lets callers raise it further for
        // sketches that need very large headline-sized text, at the cost of more atlas memory per glyph.
        constexpr int kDownsampleFactor = 1;
        constexpr int kSpreadAtlasTexels = 4;
        constexpr int kSpreadSourcePixels = kSpreadAtlasTexels * kDownsampleFactor;
        constexpr int kAtlasPaddingTexels = kSpreadAtlasTexels;

        // --- 8SSEDT (8-points signed sequential Euclidean distance transform) ---
        // Two-pass propagation of "offset to nearest seeded pixel" across a grid; used twice (once for
        // the glyph's "inside" mask, once for "outside") to build an unsigned distance transform of each,
        // which are then combined into a single signed distance field.

        struct DistanceOffset
        {
            int dx, dy;
        };

        constexpr DistanceOffset kFarOffset {16384, 16384};

        inline int squaredLength(const DistanceOffset& o)
        {
            return o.dx * o.dx + o.dy * o.dy;
        }

        class EdtGrid
        {
        public:
            EdtGrid(int width, int height)
                : m_width(width), m_height(height), m_offsets(static_cast<size_t>(width) * static_cast<size_t>(height), kFarOffset)
            {
            }

            void seed(int x, int y)
            {
                m_offsets[index(x, y)] = {0, 0};
            }

            void propagate()
            {
                for (int y = 0; y < m_height; ++y) {
                    for (int x = 0; x < m_width; ++x) {
                        compare(x, y, -1, 0);
                        compare(x, y, 0, -1);
                        compare(x, y, -1, -1);
                        compare(x, y, 1, -1);
                    }
                    for (int x = m_width - 1; x >= 0; --x) {
                        compare(x, y, 1, 0);
                    }
                }
                for (int y = m_height - 1; y >= 0; --y) {
                    for (int x = m_width - 1; x >= 0; --x) {
                        compare(x, y, 1, 0);
                        compare(x, y, 0, 1);
                        compare(x, y, 1, 1);
                        compare(x, y, -1, 1);
                    }
                    for (int x = 0; x < m_width; ++x) {
                        compare(x, y, -1, 0);
                    }
                }
            }

            float distanceAt(int x, int y) const
            {
                return std::sqrt(static_cast<float>(squaredLength(m_offsets[index(x, y)])));
            }

        private:
            size_t index(int x, int y) const
            {
                return static_cast<size_t>(y) * static_cast<size_t>(m_width) + static_cast<size_t>(x);
            }

            void compare(int x, int y, int dx, int dy)
            {
                const int nx = x + dx;
                const int ny = y + dy;
                if (nx < 0 or nx >= m_width or ny < 0 or ny >= m_height) {
                    return;
                }

                DistanceOffset candidate = m_offsets[index(nx, ny)];
                candidate.dx += dx;
                candidate.dy += dy;
                if (squaredLength(candidate) < squaredLength(m_offsets[index(x, y)])) {
                    m_offsets[index(x, y)] = candidate;
                }
            }

            int m_width, m_height;
            std::vector<DistanceOffset> m_offsets;
        };

        // Converts an 8-bit AA coverage bitmap (as produced by FT_RENDER_MODE_NORMAL) into a signed
        // distance field, encoded as bytes where 128 = glyph edge, >128 = inside, <128 = outside.
        std::vector<uint8_t> generateSDF(const uint8_t* coverage, int width, int height, int pitch)
        {
            EdtGrid insideGrid(width, height);  // holds distance to nearest INSIDE pixel
            EdtGrid outsideGrid(width, height); // holds distance to nearest OUTSIDE pixel

            for (int y = 0; y < height; ++y) {
                const uint8_t* row = coverage + static_cast<ptrdiff_t>(y) * pitch;
                for (int x = 0; x < width; ++x) {
                    if (row[x] >= 128) {
                        insideGrid.seed(x, y);
                    } else {
                        outsideGrid.seed(x, y);
                    }
                }
            }

            insideGrid.propagate();
            outsideGrid.propagate();

            std::vector<uint8_t> sdf(static_cast<size_t>(width) * static_cast<size_t>(height));
            for (int y = 0; y < height; ++y) {
                const uint8_t* row = coverage + static_cast<ptrdiff_t>(y) * pitch;
                for (int x = 0; x < width; ++x) {
                    const bool isInside = row[x] >= 128;
                    const float signedDistance = isInside ? outsideGrid.distanceAt(x, y) : -insideGrid.distanceAt(x, y);
                    const float normalized = std::clamp(signedDistance / static_cast<float>(kSpreadSourcePixels), -1.0f, 1.0f);
                    sdf[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)] =
                        static_cast<uint8_t>(std::clamp(normalized * 127.0f + 128.0f, 0.0f, 255.0f));
                }
            }
            return sdf;
        }

        // Box-filter downsamples a source-resolution SDF byte buffer by kDownsampleFactor into the
        // smaller cell that actually gets uploaded to the atlas.
        std::vector<uint8_t> downsampleSDF(const std::vector<uint8_t>& source, int srcWidth, int srcHeight, int& outWidth, int& outHeight)
        {
            outWidth = std::max(1, (srcWidth + kDownsampleFactor - 1) / kDownsampleFactor);
            outHeight = std::max(1, (srcHeight + kDownsampleFactor - 1) / kDownsampleFactor);

            std::vector<uint8_t> result(static_cast<size_t>(outWidth) * static_cast<size_t>(outHeight));
            for (int y = 0; y < outHeight; ++y) {
                for (int x = 0; x < outWidth; ++x) {
                    int sum = 0;
                    int count = 0;
                    for (int by = 0; by < kDownsampleFactor; ++by) {
                        for (int bx = 0; bx < kDownsampleFactor; ++bx) {
                            const int sx = x * kDownsampleFactor + bx;
                            const int sy = y * kDownsampleFactor + by;
                            if (sx < srcWidth and sy < srcHeight) {
                                sum += source[static_cast<size_t>(sy) * static_cast<size_t>(srcWidth) + static_cast<size_t>(sx)];
                                ++count;
                            }
                        }
                    }
                    result[static_cast<size_t>(y) * static_cast<size_t>(outWidth) + static_cast<size_t>(x)] = static_cast<uint8_t>(count > 0 ? sum / count : 0);
                }
            }
            return result;
        }
    } // namespace

    class FreeTypeHarfBuzzFont : public Font
    {
    public:
        // rasterFace and hbFace are two independent FT_Face handles opened from the same font data.
        // They must stay separate: HarfBuzz caches each glyph's advance the first time it's queried,
        // and if that first query happens after rasterFace's pixel size/glyph slot has been mutated by
        // our own SDF rasterization (FT_Set_Pixel_Sizes + FT_LOAD_RENDER in rasterizeGlyph()), the
        // cached advance comes back wrong (seen ~4x inflated on some glyphs, exact value depends on
        // whatever the glyph slot's bitmap-rendered state happened to leave behind) — and stays wrong
        // for the lifetime of the hb_font_t once cached. Giving HarfBuzz its own face that our
        // rasterization code never touches sidesteps the whole class of bug regardless of query order.
        FreeTypeHarfBuzzFont(FT_Face rasterFace, FT_Face hbFace, hb_font_t* hbFont, uint32_t atlasWidth, uint32_t atlasHeight, uint32_t sdfSourceEmPixels)
            : m_rasterFace(rasterFace), m_hbFace(hbFace), m_hbFont(hbFont),
              m_atlasTexture(loadTextureFromMemory(atlasWidth, atlasHeight, {}, TexturePixelFormat::r8)),
              m_sdfSourceEmPixels(sdfSourceEmPixels)
        {
            // Prepopulate printable ASCII so common Latin text never hits an on-demand rasterize hitch
            // or an atlas-full fallback (see packIntoAtlas()) for the overwhelmingly common case.
            for (uint32_t codepoint = 0x20; codepoint <= 0x7E; ++codepoint) {
                const uint32_t glyphIndex = FT_Get_Char_Index(m_rasterFace, codepoint);
                if (glyphIndex != 0) {
                    getGlyphMetrics(glyphIndex);
                }
            }
        }

        FreeTypeHarfBuzzFont(const FreeTypeHarfBuzzFont&) = delete;
        FreeTypeHarfBuzzFont& operator=(const FreeTypeHarfBuzzFont&) = delete;

        ~FreeTypeHarfBuzzFont() override
        {
            hb_font_destroy(m_hbFont);
            FT_Done_Face(m_hbFace);
            FT_Done_Face(m_rasterFace);
        }

        std::vector<ShapedGlyph> shape(std::string_view utf8Text) const override
        {
            hb_buffer_t* buffer = hb_buffer_create();
            hb_buffer_add_utf8(buffer, utf8Text.data(), static_cast<int>(utf8Text.size()), 0, -1);
            hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);
            hb_buffer_guess_segment_properties(buffer);
            hb_shape(m_hbFont, buffer, nullptr, 0);

            unsigned int glyphCount = 0;
            const hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buffer, &glyphCount);
            const hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer, &glyphCount);

            std::vector<ShapedGlyph> result(glyphCount);
            for (unsigned int i = 0; i < glyphCount; ++i) {
                result[i] = ShapedGlyph {
                    .glyphIndex = infos[i].codepoint,
                    .cluster = infos[i].cluster,
                    .xAdvance = static_cast<float>(positions[i].x_advance),
                    .yAdvance = static_cast<float>(positions[i].y_advance),
                    .xOffset = static_cast<float>(positions[i].x_offset),
                    .yOffset = static_cast<float>(positions[i].y_offset),
                };
            }

            hb_buffer_destroy(buffer);
            return result;
        }

        const GlyphMetrics& getGlyphMetrics(uint32_t glyphIndex) override
        {
            if (const auto it = m_glyphCache.find(glyphIndex); it != m_glyphCache.end()) {
                return it->second;
            }
            return rasterizeGlyph(glyphIndex);
        }

        std::shared_ptr<Texture> getAtlasTexture() const override
        {
            return m_atlasTexture;
        }

        float getUnitsPerEm() const override { return static_cast<float>(m_rasterFace->units_per_EM); }
        float getAscent() const override { return static_cast<float>(m_rasterFace->ascender); }
        float getDescent() const override { return static_cast<float>(-m_rasterFace->descender); }

        float getLineGap() const override
        {
            const float lineHeight = static_cast<float>(m_rasterFace->height);
            return std::max(0.0f, lineHeight - (getAscent() + getDescent()));
        }

    private:
        const GlyphMetrics& rasterizeGlyph(uint32_t glyphIndex)
        {
            // Pass 1: unscaled ink bounds, directly in font design units.
            if (FT_Load_Glyph(m_rasterFace, glyphIndex, FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING) != 0) {
                error("Font: FT_Load_Glyph() (metrics pass) failed for glyph index {}", glyphIndex);
                const auto [it, inserted] = m_glyphCache.emplace(glyphIndex, GlyphMetrics {.uvRect = {}, .bounds = {}, .hasOutline = false});
                return it->second;
            }
            const FT_Glyph_Metrics& unscaledMetrics = m_rasterFace->glyph->metrics;
            const rect2f designBounds {
                static_cast<float>(unscaledMetrics.horiBearingX),
                static_cast<float>(unscaledMetrics.horiBearingY),
                static_cast<float>(unscaledMetrics.width),
                static_cast<float>(unscaledMetrics.height),
            };

            if (designBounds.width <= 0.0f or designBounds.height <= 0.0f) {
                // No ink (space, tab, ...) — advance-only glyph, no atlas cell.
                const auto [it, inserted] = m_glyphCache.emplace(glyphIndex, GlyphMetrics {.uvRect = {}, .bounds = designBounds, .hasOutline = false});
                return it->second;
            }

            // Pass 2: AA coverage bitmap at a fixed source resolution, used to build the SDF.
            FT_Set_Pixel_Sizes(m_rasterFace, 0, m_sdfSourceEmPixels);
            if (FT_Load_Glyph(m_rasterFace, glyphIndex, FT_LOAD_RENDER | FT_LOAD_NO_HINTING) != 0) {
                error("Font: FT_Load_Glyph() (render pass) failed for glyph index {}", glyphIndex);
                const auto [it, inserted] = m_glyphCache.emplace(glyphIndex, GlyphMetrics {.uvRect = {}, .bounds = designBounds, .hasOutline = false});
                return it->second;
            }
            const FT_Bitmap& bitmap = m_rasterFace->glyph->bitmap;

            if (bitmap.width == 0 or bitmap.rows == 0) {
                const auto [it, inserted] = m_glyphCache.emplace(glyphIndex, GlyphMetrics {.uvRect = {}, .bounds = designBounds, .hasOutline = false});
                return it->second;
            }

            const std::vector<uint8_t> sourceSDF = generateSDF(bitmap.buffer, static_cast<int>(bitmap.width), static_cast<int>(bitmap.rows), bitmap.pitch);

            int cellWidth = 0;
            int cellHeight = 0;
            const std::vector<uint8_t> cellSDF = downsampleSDF(sourceSDF, static_cast<int>(bitmap.width), static_cast<int>(bitmap.rows), cellWidth, cellHeight);

            const std::optional<rect2f> uvRect = packIntoAtlas(cellSDF, cellWidth, cellHeight);
            if (not uvRect.has_value()) {
                // Atlas is full — keep the glyph's advance-only metrics rather than crashing; it just
                // won't render until the Font is constructed with a larger atlas.
                const auto [it, inserted] = m_glyphCache.emplace(glyphIndex, GlyphMetrics {.uvRect = {}, .bounds = designBounds, .hasOutline = false});
                return it->second;
            }

            const auto [it, inserted] = m_glyphCache.emplace(glyphIndex, GlyphMetrics {.uvRect = *uvRect, .bounds = designBounds, .hasOutline = true});
            return it->second;
        }

        std::optional<rect2f> packIntoAtlas(const std::vector<uint8_t>& cellSDF, int cellWidth, int cellHeight)
        {
            const uint32_t atlasWidth = m_atlasTexture->getSize().x;
            const uint32_t atlasHeight = m_atlasTexture->getSize().y;

            const uint32_t paddedWidth = static_cast<uint32_t>(cellWidth) + 2 * kAtlasPaddingTexels;
            const uint32_t paddedHeight = static_cast<uint32_t>(cellHeight) + 2 * kAtlasPaddingTexels;

            if (m_shelfX + paddedWidth > atlasWidth) {
                m_shelfX = 0;
                m_shelfY += m_shelfHeight;
                m_shelfHeight = 0;
            }
            if (m_shelfY + paddedHeight > atlasHeight) {
                error("Font: SDF atlas ({}x{}) is full; construct the Font with a larger atlasWidth/atlasHeight", atlasWidth, atlasHeight);
                return std::nullopt;
            }

            // Fill the padded cell with the "far outside" byte value, then blit the glyph SDF into its
            // center, so bilinear sampling near the glyph's edges never bleeds into a neighboring glyph.
            std::vector<uint8_t> padded(static_cast<size_t>(paddedWidth) * static_cast<size_t>(paddedHeight), 0);
            for (int y = 0; y < cellHeight; ++y) {
                for (int x = 0; x < cellWidth; ++x) {
                    const size_t dstIndex = static_cast<size_t>(y + kAtlasPaddingTexels) * paddedWidth + static_cast<size_t>(x + kAtlasPaddingTexels);
                    padded[dstIndex] = cellSDF[static_cast<size_t>(y) * static_cast<size_t>(cellWidth) + static_cast<size_t>(x)];
                }
            }

            m_atlasTexture->updateSubImage(m_shelfX, m_shelfY, paddedWidth, paddedHeight, padded);

            const rect2f uvRect {
                static_cast<float>(m_shelfX + kAtlasPaddingTexels) / static_cast<float>(atlasWidth),
                static_cast<float>(m_shelfY + kAtlasPaddingTexels) / static_cast<float>(atlasHeight),
                static_cast<float>(cellWidth) / static_cast<float>(atlasWidth),
                static_cast<float>(cellHeight) / static_cast<float>(atlasHeight),
            };

            m_shelfX += paddedWidth;
            m_shelfHeight = std::max(m_shelfHeight, paddedHeight);

            return uvRect;
        }

        FT_Face m_rasterFace;
        FT_Face m_hbFace;
        hb_font_t* m_hbFont;
        std::shared_ptr<Texture> m_atlasTexture;
        uint32_t m_sdfSourceEmPixels;
        std::unordered_map<uint32_t, GlyphMetrics> m_glyphCache;

        uint32_t m_shelfX = 0;
        uint32_t m_shelfY = 0;
        uint32_t m_shelfHeight = 0;
    };

    std::unique_ptr<Font> loadFontFromMemory(std::span<const uint8_t> data, uint32_t atlasWidth, uint32_t atlasHeight, uint32_t sdfSourceEmPixels)
    {
        FT_Face rasterFace = nullptr;
        if (FT_New_Memory_Face(freeTypeLibrary(), data.data(), static_cast<FT_Long>(data.size()), 0, &rasterFace) != 0) {
            return nullptr;
        }

        // A second, independent face for HarfBuzz — see the FreeTypeHarfBuzzFont comment for why this
        // must not be the same FT_Face our own SDF rasterization mutates via FT_Set_Pixel_Sizes().
        FT_Face hbFace = nullptr;
        if (FT_New_Memory_Face(freeTypeLibrary(), data.data(), static_cast<FT_Long>(data.size()), 0, &hbFace) != 0) {
            FT_Done_Face(rasterFace);
            return nullptr;
        }

        hb_font_t* hbFont = hb_ft_font_create(hbFace, nullptr);
        if (hbFont == nullptr) {
            FT_Done_Face(hbFace);
            FT_Done_Face(rasterFace);
            return nullptr;
        }

        // hb_ft_font_create() otherwise tracks the wrapped face's *current* pixel size dynamically
        // rather than fixing the scale to the font's design-unit em square at creation time. Pin it
        // explicitly so shape() always returns design-unit advances/offsets, matching what
        // Graphics::text()'s scale math assumes.
        hb_font_set_scale(hbFont, static_cast<int>(hbFace->units_per_EM), static_cast<int>(hbFace->units_per_EM));

        return std::make_unique<FreeTypeHarfBuzzFont>(rasterFace, hbFace, hbFont, atlasWidth, atlasHeight, sdfSourceEmPixels);
    }

    std::unique_ptr<Font> loadFontFromFile(const std::filesystem::path& filepath, uint32_t atlasWidth, uint32_t atlasHeight, uint32_t sdfSourceEmPixels)
    {
        const std::string filepathStr = filepath.string();

        FT_Face rasterFace = nullptr;
        if (FT_New_Face(freeTypeLibrary(), filepathStr.c_str(), 0, &rasterFace) != 0) {
            return nullptr;
        }

        // See loadFontFromMemory() for why HarfBuzz needs its own independent face here.
        FT_Face hbFace = nullptr;
        if (FT_New_Face(freeTypeLibrary(), filepathStr.c_str(), 0, &hbFace) != 0) {
            FT_Done_Face(rasterFace);
            return nullptr;
        }

        hb_font_t* hbFont = hb_ft_font_create(hbFace, nullptr);
        if (hbFont == nullptr) {
            FT_Done_Face(hbFace);
            FT_Done_Face(rasterFace);
            return nullptr;
        }

        hb_font_set_scale(hbFont, static_cast<int>(hbFace->units_per_EM), static_cast<int>(hbFace->units_per_EM));

        return std::make_unique<FreeTypeHarfBuzzFont>(rasterFace, hbFace, hbFont, atlasWidth, atlasHeight, sdfSourceEmPixels);
    }
} // namespace p5

namespace p5
{
    namespace detail
    {
        namespace
        {
            // letterSpacingDesignUnits is added after every glyph (including the last), so callers that
            // sum multiple shapedWidth() results (e.g. word + trailing space) don't double- or under-count it.
            float shapedWidth(const std::vector<ShapedGlyph>& glyphs, float letterSpacingDesignUnits)
            {
                float width = 0.0f;
                for (const auto& g : glyphs) width += g.xAdvance + letterSpacingDesignUnits;
                return width;
            }

            void appendWordWrappedLines(const Font& font, std::string_view segment, float maxWidthDesignUnits, float letterSpacingDesignUnits, std::vector<ShapedLine>& outLines)
            {
                const std::vector<ShapedGlyph> spaceGlyphs = font.shape(" ");
                const float spaceWidth = shapedWidth(spaceGlyphs, letterSpacingDesignUnits);

                std::vector<ShapedGlyph> currentGlyphs;
                float currentWidth = 0.0f;
                bool lineHasWord = false;

                size_t pos = 0;
                while (pos < segment.size()) {
                    while (pos < segment.size() and segment[pos] == ' ') ++pos;
                    const size_t wordStart = pos;
                    while (pos < segment.size() and segment[pos] != ' ') ++pos;
                    if (wordStart == pos) break; // only trailing spaces remain

                    const std::string_view word = segment.substr(wordStart, pos - wordStart);
                    const std::vector<ShapedGlyph> wordGlyphs = font.shape(word);
                    const float wordWidth = shapedWidth(wordGlyphs, letterSpacingDesignUnits);

                    const float candidateWidth = lineHasWord ? currentWidth + spaceWidth + wordWidth : wordWidth;
                    if (lineHasWord and candidateWidth > maxWidthDesignUnits) {
                        outLines.push_back({std::move(currentGlyphs), currentWidth});
                        currentGlyphs.clear();
                        currentWidth = 0.0f;
                        lineHasWord = false;
                    }

                    if (lineHasWord) {
                        currentGlyphs.insert(currentGlyphs.end(), spaceGlyphs.begin(), spaceGlyphs.end());
                        currentWidth += spaceWidth;
                    }
                    currentGlyphs.insert(currentGlyphs.end(), wordGlyphs.begin(), wordGlyphs.end());
                    currentWidth += wordWidth;
                    lineHasWord = true;
                }

                outLines.push_back({std::move(currentGlyphs), currentWidth});
            }

            void appendCharacterWrappedLines(const Font& font, std::string_view segment, float maxWidthDesignUnits, float letterSpacingDesignUnits, std::vector<ShapedLine>& outLines)
            {
                size_t start = 0;
                for (;;) {
                    const std::string_view remaining = segment.substr(start);
                    const std::vector<ShapedGlyph> glyphs = font.shape(remaining);
                    if (glyphs.empty()) {
                        outLines.push_back({{}, 0.0f});
                        return;
                    }

                    float width = 0.0f;
                    size_t cutGlyphCount = glyphs.size();
                    size_t cutByteOffset = remaining.size();
                    size_t lastClusterBoundary = 0;

                    for (size_t i = 0; i < glyphs.size(); ++i) {
                        if (i > 0 and glyphs[i].cluster != glyphs[i - 1].cluster) {
                            lastClusterBoundary = i;
                        }

                        const float nextWidth = width + glyphs[i].xAdvance + letterSpacingDesignUnits;
                        if (nextWidth > maxWidthDesignUnits and i > 0) {
                            if (lastClusterBoundary > 0) {
                                cutGlyphCount = lastClusterBoundary;
                                cutByteOffset = glyphs[lastClusterBoundary].cluster;
                            } else {
                                // The overflowing content is all one cluster (e.g. a ligature) — keep it
                                // whole on this line rather than cutting a zero-length line forever.
                                cutGlyphCount = i;
                                cutByteOffset = glyphs[i].cluster;
                            }
                            break;
                        }
                        width = nextWidth;
                    }

                    if (cutGlyphCount == glyphs.size()) {
                        outLines.push_back({glyphs, width});
                        return;
                    }

                    std::vector<ShapedGlyph> lineGlyphs(glyphs.begin(), glyphs.begin() + static_cast<std::ptrdiff_t>(cutGlyphCount));
                    const float lineWidth = shapedWidth(lineGlyphs, letterSpacingDesignUnits);
                    outLines.push_back({std::move(lineGlyphs), lineWidth});

                    if (cutByteOffset == 0) {
                        return; // guard against a zero-length cut looping forever; shouldn't occur
                    }
                    start += cutByteOffset;
                }
            }
        } // namespace

        LineLayout layoutLines(const Font& font, float size, std::string_view str, TextWrap wrap, float maxWidth, float letterSpacing)
        {
            LineLayout layout;
            layout.unitsPerEm = font.getUnitsPerEm();
            const float scale = size / layout.unitsPerEm;
            const float maxWidthDesignUnits = maxWidth / scale;
            const float letterSpacingDesignUnits = letterSpacing / scale;

            size_t start = 0;
            for (;;) {
                const size_t newline = str.find('\n', start);
                const std::string_view segment = str.substr(start, newline == std::string_view::npos ? std::string_view::npos : newline - start);

                if (wrap == TextWrap::none or maxWidth <= 0.0f) {
                    std::vector<ShapedGlyph> glyphs = font.shape(segment);
                    const float width = shapedWidth(glyphs, letterSpacingDesignUnits);
                    layout.lines.push_back({std::move(glyphs), width});
                } else if (wrap == TextWrap::word) {
                    appendWordWrappedLines(font, segment, maxWidthDesignUnits, letterSpacingDesignUnits, layout.lines);
                } else {
                    appendCharacterWrappedLines(font, segment, maxWidthDesignUnits, letterSpacingDesignUnits, layout.lines);
                }

                if (newline == std::string_view::npos) break;
                start = newline + 1;
            }

            return layout;
        }
    } // namespace detail

    float textWidth(const Font& font, float size, std::string_view str, float letterSpacing)
    {
        const float scale = size / font.getUnitsPerEm();
        const std::vector<ShapedGlyph> glyphs = font.shape(str);
        float width = 0.0f;
        for (const ShapedGlyph& g : glyphs) {
            width += g.xAdvance;
        }
        return width * scale + static_cast<float>(glyphs.size()) * letterSpacing;
    }

    rect2f textBounds(const Font& font, float size, std::string_view str, TextWrap wrap, float maxWidth, float letterSpacing)
    {
        const detail::LineLayout layout = detail::layoutLines(font, size, str, wrap, maxWidth, letterSpacing);
        const float scale = size / layout.unitsPerEm;

        float blockWidth = 0.0f;
        for (const detail::ShapedLine& line : layout.lines) {
            blockWidth = std::max(blockWidth, line.width * scale);
        }

        const float leading = (font.getAscent() + font.getDescent() + font.getLineGap()) * scale;
        const float blockTop = font.getAscent() * scale;
        const float blockHeight = blockTop + static_cast<float>(layout.lines.size() - 1) * leading + font.getDescent() * scale;

        return rect2f {0.0f, 0.0f, blockWidth, blockHeight};
    }
} // namespace p5
