#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/text_layout.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include <algorithm>
#include <mutex>
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

        // FreeType documents that its own memory-management bookkeeping inside a single FT_Library is
        // not thread-safe: concurrent FT_New_*_Face()/FT_Done_Face() calls against the shared library
        // above (every Font in the process uses the same one) race on that internal state. Serializing
        // just the face-lifetime calls -- not glyph rasterization on an already-created FT_Face, which
        // per-Font state already keeps single-threaded via each Font's own call pattern -- is enough to
        // make concurrent loadFontFromMemory()/loadFontFromFile()/~FreeTypeHarfBuzzFont() calls safe.
        std::mutex& freeTypeMutex()
        {
            static std::mutex mutex;
            return mutex;
        }

        // Glyphs are rasterized once as a plain 8-bit antialiased coverage bitmap (FreeType's
        // FT_RENDER_MODE_NORMAL grayscale output -- the same kind of AA a software text rasterizer like
        // Java2D's produces) at a fixed size, and that single cached bitmap is reused for every display
        // size, exactly like Processing's PFont/FontTexture: one bitmap per glyph, bilinear-scaled by
        // the GPU for whatever textSize() is requested. No distance field, no spread, no per-size
        // re-rasterization.
        //
        // The trade-off is the same one Processing accepts: text scaled well above atlasEmPixels will
        // visibly soften, since there's nothing beyond GPU bilinear filtering to keep edges crisp past
        // the baked resolution. Callers who need large headline-sized (or animated/scaled-up) text
        // should pass an atlasEmPixels close to the largest size they'll actually display -- mirroring
        // Processing's own createFont(name, size) advice to "create the font at the largest size
        // needed, then scale down."
        constexpr int kAtlasPaddingTexels = 1; // guards against bilinear bleed between neighboring atlas cells

        // Copies an FT_Bitmap's coverage rows (respecting `pitch`, which can exceed `width` for row
        // alignment) into a tightly packed, row-major buffer the atlas packer can blit directly.
        std::vector<uint8_t> packCoverageBitmap(const uint8_t* buffer, int width, int height, int pitch)
        {
            std::vector<uint8_t> result(static_cast<size_t>(width) * static_cast<size_t>(height));
            for (int y = 0; y < height; ++y) {
                const uint8_t* row = buffer + static_cast<ptrdiff_t>(y) * pitch;
                std::copy(row, row + width, result.begin() + static_cast<ptrdiff_t>(y) * width);
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
        // our own glyph rasterization (FT_Set_Pixel_Sizes + FT_LOAD_RENDER in rasterizeGlyph()), the
        // cached advance comes back wrong (seen ~4x inflated on some glyphs, exact value depends on
        // whatever the glyph slot's bitmap-rendered state happened to leave behind) — and stays wrong
        // for the lifetime of the hb_font_t once cached. Giving HarfBuzz its own face that our
        // rasterization code never touches sidesteps the whole class of bug regardless of query order.
        FreeTypeHarfBuzzFont(FT_Face rasterFace, FT_Face hbFace, hb_font_t* hbFont, uint32_t atlasWidth, uint32_t atlasHeight, uint32_t atlasEmPixels)
            : m_rasterFace(rasterFace), m_hbFace(hbFace), m_hbFont(hbFont),
              m_atlasTexture(loadTextureFromMemory(atlasWidth, atlasHeight, {}, TexturePixelFormat::r8)),
              m_atlasEmPixels(atlasEmPixels)
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
            std::lock_guard<std::mutex> lock(freeTypeMutex());
            FT_Done_Face(m_hbFace);
            FT_Done_Face(m_rasterFace);
        }

        std::vector<ShapedGlyph> shape(std::string_view utf8Text) const override
        {
            hb_buffer_t* buffer = hb_buffer_create();
            hb_buffer_add_utf8(buffer, utf8Text.data(), static_cast<int>(utf8Text.size()), 0, -1);
            // Let HarfBuzz infer direction (along with script/language) from the buffer's actual
            // Unicode content instead of hardcoding LTR -- guess_segment_properties() only fills in
            // fields that aren't already set, so a prior explicit set_direction(LTR) here silently
            // forced every RTL script (Arabic, Hebrew, ...) to shape left-to-right instead.
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

        float getUnitsPerEm() const override
        {
            // units_per_EM is only meaningful for scalable outline formats; FreeType leaves it 0 for
            // bitmap-only formats (.pcf/.bdf/.fon, all valid loadFontFromFile() inputs). Every caller
            // of this (Graphics::text()/textWidth()/textBounds(), layoutLines()) divides textSize by
            // it, so returning 0 here would propagate Inf/NaN into vertex positions instead of just
            // rendering bitmap glyphs at an imprecise-but-finite scale. 1000 is the common TrueType/
            // OpenType em square, and a reasonable finite fallback for the formats that lack one.
            const FT_UShort unitsPerEm = m_rasterFace->units_per_EM;
            return unitsPerEm != 0 ? static_cast<float>(unitsPerEm) : 1000.0f;
        }
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

            // Pass 2: plain AA coverage bitmap at the fixed atlas bake resolution -- the same bitmap is
            // reused (GPU-bilinear-scaled) for every requested display size, see the comment on
            // kAtlasPaddingTexels above.
            FT_Set_Pixel_Sizes(m_rasterFace, 0, m_atlasEmPixels);
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

            const int cellWidth = static_cast<int>(bitmap.width);
            const int cellHeight = static_cast<int>(bitmap.rows);
            const std::vector<uint8_t> cellCoverage = packCoverageBitmap(bitmap.buffer, cellWidth, cellHeight, bitmap.pitch);

            const std::optional<rect2f> uvRect = packIntoAtlas(cellCoverage, cellWidth, cellHeight);
            if (not uvRect.has_value()) {
                // Atlas is full — keep the glyph's advance-only metrics rather than crashing; it just
                // won't render until the Font is constructed with a larger atlas.
                const auto [it, inserted] = m_glyphCache.emplace(glyphIndex, GlyphMetrics {.uvRect = {}, .bounds = designBounds, .hasOutline = false});
                return it->second;
            }

            const auto [it, inserted] = m_glyphCache.emplace(glyphIndex, GlyphMetrics {.uvRect = *uvRect, .bounds = designBounds, .hasOutline = true});
            return it->second;
        }

        std::optional<rect2f> packIntoAtlas(const std::vector<uint8_t>& cellCoverage, int cellWidth, int cellHeight)
        {
            const uint32_t atlasWidth = m_atlasTexture->size.x;
            const uint32_t atlasHeight = m_atlasTexture->size.y;

            const uint32_t paddedWidth = static_cast<uint32_t>(cellWidth) + 2 * kAtlasPaddingTexels;
            const uint32_t paddedHeight = static_cast<uint32_t>(cellHeight) + 2 * kAtlasPaddingTexels;

            // A glyph that doesn't fit even on an empty shelf never will, regardless of how many times
            // the wrap-to-new-line logic below retries it -- without this check it would wrap to
            // m_shelfX=0, still not fit (paddedWidth > atlasWidth), then fall through to
            // updateSubImage() silently no-op'ing on its own out-of-bounds check while this function
            // still cached a uvRect claiming success and permanently burned that shelf row.
            if (paddedWidth > atlasWidth or paddedHeight > atlasHeight) {
                error("Font: glyph cell ({}x{} padded) does not fit in the glyph atlas ({}x{}); construct the Font with a larger atlasWidth/atlasHeight or a smaller atlasEmPixels", paddedWidth, paddedHeight, atlasWidth, atlasHeight);
                return std::nullopt;
            }

            if (m_shelfX + paddedWidth > atlasWidth) {
                m_shelfX = 0;
                m_shelfY += m_shelfHeight;
                m_shelfHeight = 0;
            }
            if (m_shelfY + paddedHeight > atlasHeight) {
                error("Font: glyph atlas ({}x{}) is full; construct the Font with a larger atlasWidth/atlasHeight", atlasWidth, atlasHeight);
                return std::nullopt;
            }

            // Fill the padded cell with zero coverage, then blit the glyph bitmap into its center, so
            // bilinear sampling near the glyph's edges never bleeds into a neighboring glyph.
            std::vector<uint8_t> padded(static_cast<size_t>(paddedWidth) * static_cast<size_t>(paddedHeight), 0);
            for (int y = 0; y < cellHeight; ++y) {
                for (int x = 0; x < cellWidth; ++x) {
                    const size_t dstIndex = static_cast<size_t>(y + kAtlasPaddingTexels) * paddedWidth + static_cast<size_t>(x + kAtlasPaddingTexels);
                    padded[dstIndex] = cellCoverage[static_cast<size_t>(y) * static_cast<size_t>(cellWidth) + static_cast<size_t>(x)];
                }
            }

            updateSubImage(*m_atlasTexture, m_shelfX, m_shelfY, paddedWidth, paddedHeight, padded);

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
        uint32_t m_atlasEmPixels;
        std::unordered_map<uint32_t, GlyphMetrics> m_glyphCache;

        uint32_t m_shelfX = 0;
        uint32_t m_shelfY = 0;
        uint32_t m_shelfHeight = 0;
    };

    namespace
    {
        // Transfers ownership of already-created rasterFace/hbFace/hbFont into a new
        // FreeTypeHarfBuzzFont, freeing all three instead of leaking them if construction throws (e.g.
        // bad_alloc from the ASCII-prepopulation loop in its constructor body) -- at that point
        // ownership never successfully transferred, and FreeTypeHarfBuzzFont's own destructor never
        // runs for an object whose constructor didn't complete.
        std::unique_ptr<Font> makeFreeTypeHarfBuzzFont(FT_Face rasterFace, FT_Face hbFace, hb_font_t* hbFont, uint32_t atlasWidth, uint32_t atlasHeight, uint32_t atlasEmPixels)
        {
            try {
                return std::make_unique<FreeTypeHarfBuzzFont>(rasterFace, hbFace, hbFont, atlasWidth, atlasHeight, atlasEmPixels);
            } catch (...) {
                hb_font_destroy(hbFont);
                std::lock_guard<std::mutex> lock(freeTypeMutex());
                FT_Done_Face(hbFace);
                FT_Done_Face(rasterFace);
                throw;
            }
        }
    } // namespace

    std::unique_ptr<Font> loadFontFromMemory(std::span<const uint8_t> data, uint32_t atlasWidth, uint32_t atlasHeight, uint32_t atlasEmPixels)
    {
        FT_Face rasterFace = nullptr;
        FT_Face hbFace = nullptr;
        {
            std::lock_guard<std::mutex> lock(freeTypeMutex());
            if (FT_New_Memory_Face(freeTypeLibrary(), data.data(), static_cast<FT_Long>(data.size()), 0, &rasterFace) != 0) {
                return nullptr;
            }

            // A second, independent face for HarfBuzz — see the FreeTypeHarfBuzzFont comment for why this
            // must not be the same FT_Face our own glyph rasterization mutates via FT_Set_Pixel_Sizes().
            if (FT_New_Memory_Face(freeTypeLibrary(), data.data(), static_cast<FT_Long>(data.size()), 0, &hbFace) != 0) {
                FT_Done_Face(rasterFace);
                return nullptr;
            }
        }

        hb_font_t* hbFont = hb_ft_font_create(hbFace, nullptr);
        if (hbFont == nullptr) {
            std::lock_guard<std::mutex> lock(freeTypeMutex());
            FT_Done_Face(hbFace);
            FT_Done_Face(rasterFace);
            return nullptr;
        }

        // hb_ft_font_create() otherwise tracks the wrapped face's *current* pixel size dynamically
        // rather than fixing the scale to the font's design-unit em square at creation time. Pin it
        // explicitly so shape() always returns design-unit advances/offsets, matching what
        // Graphics::text()'s scale math assumes.
        hb_font_set_scale(hbFont, static_cast<int>(hbFace->units_per_EM), static_cast<int>(hbFace->units_per_EM));

        return makeFreeTypeHarfBuzzFont(rasterFace, hbFace, hbFont, atlasWidth, atlasHeight, atlasEmPixels);
    }

    std::unique_ptr<Font> loadFontFromFile(const std::filesystem::path& filepath, uint32_t atlasWidth, uint32_t atlasHeight, uint32_t atlasEmPixels)
    {
        const std::string filepathStr = filepath.string();

        FT_Face rasterFace = nullptr;
        FT_Face hbFace = nullptr;
        {
            std::lock_guard<std::mutex> lock(freeTypeMutex());
            if (FT_New_Face(freeTypeLibrary(), filepathStr.c_str(), 0, &rasterFace) != 0) {
                return nullptr;
            }

            // See loadFontFromMemory() for why HarfBuzz needs its own independent face here.
            if (FT_New_Face(freeTypeLibrary(), filepathStr.c_str(), 0, &hbFace) != 0) {
                FT_Done_Face(rasterFace);
                return nullptr;
            }
        }

        hb_font_t* hbFont = hb_ft_font_create(hbFace, nullptr);
        if (hbFont == nullptr) {
            std::lock_guard<std::mutex> lock(freeTypeMutex());
            FT_Done_Face(hbFace);
            FT_Done_Face(rasterFace);
            return nullptr;
        }

        hb_font_set_scale(hbFont, static_cast<int>(hbFace->units_per_EM), static_cast<int>(hbFace->units_per_EM));

        return makeFreeTypeHarfBuzzFont(rasterFace, hbFace, hbFont, atlasWidth, atlasHeight, atlasEmPixels);
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
                // Character wrap exists specifically for runs with no natural (space/newline) break --
                // e.g. a long URL or identifier -- so `segment` can be arbitrarily long here. Re-shaping
                // the entire remaining tail from scratch for every line found (as a naive version of
                // this does) costs O(remaining length) per line, i.e. O(segment.size()^2) overall for a
                // single unbroken long line. Shape only a bounded prefix window instead, doubling it
                // only when the whole window's glyphs still fit under maxWidthDesignUnits (meaning the
                // real cut point lies further out than shaped so far, not that there isn't one) --
                // total bytes shaped across one line's doubling attempts stays a small constant factor
                // over that line's own length, keeping the function close to O(segment.size()) overall.
                // Trade-off: a cut landing exactly where a window ends could shape marginally differently
                // than shaping the full remainder would have (HarfBuzz losing context past the window
                // edge for ligatures/kerning at the boundary) -- the same kind of approximation this
                // function already makes by re-shaping fresh at every line break rather than shaping the
                // full segment once and slicing it.
                constexpr size_t kInitialWindowBytes = 64;

                size_t start = 0;
                for (;;) {
                    const std::string_view remaining = segment.substr(start);

                    size_t windowBytes = kInitialWindowBytes;
                    std::vector<ShapedGlyph> glyphs;
                    std::string_view window;
                    bool windowCoversRemainder;
                    size_t cutGlyphCount;
                    size_t cutByteOffset;

                    for (;;) {
                        windowCoversRemainder = windowBytes >= remaining.size();
                        window = windowCoversRemainder ? remaining : remaining.substr(0, windowBytes);
                        glyphs = font.shape(window);

                        if (glyphs.empty()) {
                            outLines.push_back({{}, 0.0f});
                            return;
                        }

                        float width = 0.0f;
                        cutGlyphCount = glyphs.size();
                        cutByteOffset = window.size();
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

                        if (cutGlyphCount != glyphs.size() or windowCoversRemainder) {
                            break; // found a genuine cut inside this window, or shaped everything there is
                        }
                        windowBytes *= 2; // whole window still fits; the real cut point lies further out
                    }

                    if (cutGlyphCount == glyphs.size()) {
                        const float width = shapedWidth(glyphs, letterSpacingDesignUnits);
                        outLines.push_back({std::move(glyphs), width});
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
