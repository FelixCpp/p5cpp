#include <p5cpp/graphics/font.hpp>
#include <p5cpp/application/logging.hpp>

#include <glad/glad.h>
#include <freetype/freetype.h>
#include <freetype/ftoutln.h>
#include <hb.h>
#include <hb-ft.h>

#include <unordered_map>
#include <optional>
#include <vector>
#include <string>
#include <string_view>
#include <cassert>
#include <cmath>

namespace p5cpp
{
    // Internal-only - not part of the public API (Font's public surface returns
    // ShapedGlyph, not these) since nothing outside FontData's own shaping pipeline
    // consumes them anymore.
    struct GlyphRegion
    {
        int2 size;
        float_rect uvRect;
    };

    struct Glyph
    {
        GlyphRegion region;
        int2 bearing;
        float advanceX;
        size_t glyphAtlasIndex;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct BinPackingStrategy
    {
        virtual ~BinPackingStrategy() = default;
        virtual std::optional<int_rect> insert(int width, int height) = 0;
        virtual void reset() = 0;
    };
} // namespace p5cpp

namespace p5cpp
{
    class MaxRectsBinPacking : public BinPackingStrategy
    {
    public:
        explicit MaxRectsBinPacking(int binWidth, int binHeight)
            : m_binWidth(binWidth), m_binHeight(binHeight)
        {
            m_freeRects.push_back({0, 0, binWidth, binHeight});
        }

        std::optional<int_rect> insert(int width, int height) override
        {
            if (width <= 0 || height <= 0) {
                return int_rect {0, 0, width, height};
            }

            const std::optional<int_rect> placed = findBestShortSideFit(width, height);
            if (!placed.has_value()) {
                return std::nullopt;
            }

            splitFreeRects(placed.value());
            pruneFreeRects();

            return placed;
        }

        void reset() override
        {
            m_freeRects.clear();
            m_freeRects.push_back({0, 0, m_binWidth, m_binHeight});
        }

    private:
        // Best Short Side Fit: places the rectangle where the smaller leftover side is minimized.
        std::optional<int_rect> findBestShortSideFit(int width, int height) const
        {
            std::optional<int_rect> bestRect;
            int bestShortSide = std::numeric_limits<int>::max();
            int bestLongSide = std::numeric_limits<int>::max();

            for (const int_rect& freeRect : m_freeRects) {
                if (freeRect.width >= width && freeRect.height >= height) {
                    const int leftoverShort = std::min(freeRect.width - width, freeRect.height - height);
                    const int leftoverLong = std::max(freeRect.width - width, freeRect.height - height);

                    if (leftoverShort < bestShortSide ||
                        (leftoverShort == bestShortSide && leftoverLong < bestLongSide)) {
                        bestRect = int_rect {freeRect.left, freeRect.top, width, height};
                        bestShortSide = leftoverShort;
                        bestLongSide = leftoverLong;
                    }
                }
            }

            return bestRect;
        }

        // After placing a rectangle, split all free rectangles that overlap with it.
        void splitFreeRects(const int_rect& placed)
        {
            std::vector<int_rect> newFreeRects;

            for (int i = 0; i < static_cast<int>(m_freeRects.size()); ++i) {
                if (!overlaps(m_freeRects[i], placed)) {
                    continue;
                }

                const int_rect& free = m_freeRects[i];

                // Left slice
                if (placed.left > free.left) {
                    newFreeRects.push_back({free.left, free.top, placed.left - free.left, free.height});
                }
                // Right slice
                if (placed.left + placed.width < free.left + free.width) {
                    newFreeRects.push_back({placed.left + placed.width, free.top, free.left + free.width - (placed.left + placed.width), free.height});
                }
                // Top slice
                if (placed.top > free.top) {
                    newFreeRects.push_back({free.left, free.top, free.width, placed.top - free.top});
                }
                // Bottom slice
                if (placed.top + placed.height < free.top + free.height) {
                    newFreeRects.push_back({free.left, placed.top + placed.height, free.width, free.top + free.height - (placed.top + placed.height)});
                }

                // Remove the split free rect by swapping with the last element.
                m_freeRects[i] = m_freeRects.back();
                m_freeRects.pop_back();
                --i;
            }

            for (int_rect& r : newFreeRects) {
                m_freeRects.push_back(r);
            }
        }

        // Remove free rectangles that are fully contained within another free rectangle.
        void pruneFreeRects()
        {
            for (int i = 0; i < static_cast<int>(m_freeRects.size()); ++i) {
                for (int j = i + 1; j < static_cast<int>(m_freeRects.size()); ++j) {
                    if (contains(m_freeRects[j], m_freeRects[i])) {
                        m_freeRects[i] = m_freeRects.back();
                        m_freeRects.pop_back();
                        --i;
                        break;
                    }
                    if (contains(m_freeRects[i], m_freeRects[j])) {
                        m_freeRects[j] = m_freeRects.back();
                        m_freeRects.pop_back();
                        --j;
                    }
                }
            }
        }

        static bool overlaps(const int_rect& a, const int_rect& b)
        {
            return a.left < b.left + b.width &&
                   a.left + a.width > b.left &&
                   a.top < b.top + b.height &&
                   a.top + a.height > b.top;
        }

        // Returns true if 'outer' ful.top contains 'inner'.
        static bool contains(const int_rect& outer, const int_rect& inner)
        {
            return inner.left >= outer.left &&
                   inner.top >= outer.top &&
                   inner.left + inner.width <= outer.left + outer.width &&
                   inner.top + inner.height <= outer.top + outer.height;
        }

        int m_binWidth;
        int m_binHeight;
        std::vector<int_rect> m_freeRects;
    };
} // namespace p5cpp

namespace p5cpp
{
    class GlyphAtlas
    {
    public:
        explicit GlyphAtlas(int width, int height, int paddingX, int paddingY)
            : m_width(width),
              m_height(height),
              m_paddingX(paddingX),
              m_paddingY(paddingY),
              m_texture(makeGlyphAtlasTexture(width, height)),
              m_packingStrategy(std::make_unique<MaxRectsBinPacking>(width, height))
        {
        }

        ~GlyphAtlas()
        {
            unload(m_texture);
        }

        std::optional<GlyphRegion> store(int bitmapWidth, int bitmapHeight, std::span<const uint8_t> bitmapData)
        {
            // A zero-sized glyph can be used for whitespace characters. Instead of returning failure, we can return a valid region with zero size.
            if (bitmapWidth <= 0 || bitmapHeight <= 0) {
                return GlyphRegion {
                    .size = int2 {bitmapWidth, bitmapHeight},
                    .uvRect = {0, 0, 0, 0},
                };
            }

            const std::optional<int_rect> placed = m_packingStrategy->insert(bitmapWidth + m_paddingX, bitmapHeight + m_paddingY);
            if (not placed.has_value()) {
                return std::nullopt;
            }

            updateRegion(m_texture, placed->left, placed->top, bitmapWidth, bitmapHeight, bitmapData);

            const float uvLeft = static_cast<float>(placed->left) / static_cast<float>(m_width);
            const float uvTop = static_cast<float>(placed->top) / static_cast<float>(m_height);
            const float uvRight = static_cast<float>(placed->left + bitmapWidth) / static_cast<float>(m_width);
            const float uvBottom = static_cast<float>(placed->top + bitmapHeight) / static_cast<float>(m_height);

            return GlyphRegion {
                .size = int2 {bitmapWidth, bitmapHeight},
                .uvRect = {
                    uvLeft,
                    uvTop,
                    uvRight - uvLeft,
                    uvBottom - uvTop,
                },
            };
        }

        const Texture* getTexture() const
        {
            return &m_texture;
        }

    private:
        int m_width;
        int m_height;
        int m_paddingX;
        int m_paddingY;

        Texture m_texture;

        std::unique_ptr<BinPackingStrategy> m_packingStrategy;
    };
} // namespace p5cpp

namespace p5cpp
{
    static struct FreetypeInitializer
    {
        FreetypeInitializer()
        {
            FT_Init_FreeType(&library);
        }

        ~FreetypeInitializer()
        {
            FT_Done_FreeType(library);
        }

        FT_Library library;
    } freetype;
} // namespace p5cpp

namespace p5cpp
{
    struct FreetypeDeleter
    {
        void operator()(FT_Face face) const
        {
            FT_Done_Face(face);
        }
    };

    using FreetypeFace = std::unique_ptr<std::remove_pointer_t<FT_Face>, FreetypeDeleter>;
} // namespace p5cpp

namespace p5cpp
{
    struct FontMetricsCacheKey
    {
        int textSize;

        bool operator==(const FontMetricsCacheKey& other) const
        {
            return textSize == other.textSize;
        }
    };

    struct FontMetricsCacheKeyHasher
    {
        std::size_t operator()(const FontMetricsCacheKey& key) const
        {
            return std::hash<int> {}(key.textSize);
        }
    };

    class FontMetricsCache
    {
    public:
        const FontMetrics* get(int textSize) const
        {
            const FontMetricsCacheKey key {
                .textSize = textSize,
            };

            const auto itr = m_cache.find(key);
            if (itr != m_cache.end()) {
                return &itr->second;
            }

            return nullptr;
        }

        const FontMetrics* put(int textSize, const FontMetrics& metrics)
        {
            FontMetricsCacheKey key {textSize};
            const auto insertion = m_cache.emplace(std::make_pair(key, metrics));
            return &insertion.first->second;
        }

    private:
        std::unordered_map<FontMetricsCacheKey, FontMetrics, FontMetricsCacheKeyHasher> m_cache;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct GlyphByIdCacheKey
    {
        uint32_t glyphId;
        int textSize;

        bool operator==(const GlyphByIdCacheKey& other) const
        {
            return glyphId == other.glyphId && textSize == other.textSize;
        }
    };

    struct GlyphByIdCacheKeyHasher
    {
        std::size_t operator()(const GlyphByIdCacheKey& key) const
        {
            std::size_t h1 = std::hash<uint32_t> {}(key.glyphId);
            std::size_t h2 = std::hash<int> {}(key.textSize);
            return h1 ^ (h2 << 1);
        }
    };

    class GlyphByIdCache
    {
    public:
        const Glyph* get(uint32_t glyphId, int textSize) const
        {
            const GlyphByIdCacheKey key {.glyphId = glyphId, .textSize = textSize};
            const auto itr = m_cache.find(key);
            if (itr != m_cache.end()) {
                return &itr->second;
            }

            return nullptr;
        }

        const Glyph* put(uint32_t glyphId, int textSize, const Glyph& glyph)
        {
            GlyphByIdCacheKey key {glyphId, textSize};
            const auto insertion = m_cache.emplace(std::make_pair(key, glyph));
            return &insertion.first->second;
        }

    private:
        std::unordered_map<GlyphByIdCacheKey, Glyph, GlyphByIdCacheKeyHasher> m_cache;
    };
} // namespace p5cpp

namespace p5cpp
{
    // Caches the result of a full HarfBuzz shaping pass (shape() re-runs hb_shape() — bidi,
    // ligatures, kerning, contextual alternates — from scratch on every call otherwise), keyed
    // by the exact text content and size. Independent of letterSpacing/wrap/align, which are
    // applied downstream by the caller using the shaped glyph list's xAdvance values.
    struct ShapedTextCacheKey
    {
        std::string text;
        int textSize;

        bool operator==(const ShapedTextCacheKey& other) const
        {
            return textSize == other.textSize && text == other.text;
        }
    };

    struct ShapedTextCacheKeyHasher
    {
        std::size_t operator()(const ShapedTextCacheKey& key) const
        {
            std::size_t h1 = std::hash<std::string> {}(key.text);
            std::size_t h2 = std::hash<int> {}(key.textSize);
            return h1 ^ (h2 << 1);
        }
    };

    class ShapedTextCache
    {
    public:
        const std::vector<ShapedGlyph>* get(std::string_view text, int textSize) const
        {
            const ShapedTextCacheKey key {.text = std::string(text), .textSize = textSize};
            const auto itr = m_cache.find(key);
            if (itr != m_cache.end()) {
                return &itr->second;
            }

            return nullptr;
        }

        const std::vector<ShapedGlyph>* put(std::string_view text, int textSize, std::vector<ShapedGlyph> shaped)
        {
            ShapedTextCacheKey key {std::string(text), textSize};
            const auto insertion = m_cache.insert_or_assign(std::move(key), std::move(shaped));
            return &insertion.first->second;
        }

    private:
        std::unordered_map<ShapedTextCacheKey, std::vector<ShapedGlyph>, ShapedTextCacheKeyHasher> m_cache;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct HbFontDeleter
    {
        void operator()(hb_font_t* font) const
        {
            hb_font_destroy(font);
        }
    };

    using HbFont = std::unique_ptr<hb_font_t, HbFontDeleter>;
} // namespace p5cpp

namespace p5cpp
{
    // Shared state for FT_Outline_Decompose callbacks below: converts FreeType's y-up,
    // glyph-local 26.6 fixed-point coordinates into y-down world points anchored at `penOffset`
    // (the glyph's pen position), and flattens conic/cubic curve segments into line segments.
    struct OutlineDecomposeContext
    {
        std::vector<TextContour>* contours = nullptr;
        TextContour* current = nullptr;
        float2 penOffset {0.0f, 0.0f};
        float2 lastPoint {0.0f, 0.0f}; // last point in FreeType-local space (y-up), for curve flattening
        int curveDetail = 8;

        static float2 fromFixed(const FT_Vector& v)
        {
            return float2 {static_cast<float>(v.x) / 64.0f, static_cast<float>(v.y) / 64.0f};
        }

        float2 toWorld(const float2& local) const
        {
            return float2 {penOffset.x + local.x, penOffset.y - local.y};
        }
    };

    static int outlineMoveTo(const FT_Vector* to, void* user)
    {
        auto* ctx = static_cast<OutlineDecomposeContext*>(user);
        ctx->contours->emplace_back();
        ctx->current = &ctx->contours->back();
        ctx->lastPoint = OutlineDecomposeContext::fromFixed(*to);
        ctx->current->push_back(ctx->toWorld(ctx->lastPoint));
        return 0;
    }

    static int outlineLineTo(const FT_Vector* to, void* user)
    {
        auto* ctx = static_cast<OutlineDecomposeContext*>(user);
        ctx->lastPoint = OutlineDecomposeContext::fromFixed(*to);
        ctx->current->push_back(ctx->toWorld(ctx->lastPoint));
        return 0;
    }

    static int outlineConicTo(const FT_Vector* control, const FT_Vector* to, void* user)
    {
        auto* ctx = static_cast<OutlineDecomposeContext*>(user);
        const float2 p0 = ctx->lastPoint;
        const float2 c = OutlineDecomposeContext::fromFixed(*control);
        const float2 p1 = OutlineDecomposeContext::fromFixed(*to);

        for (int i = 1; i <= ctx->curveDetail; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(ctx->curveDetail);
            const float mt = 1.0f - t;
            const float2 pt = p0 * (mt * mt) + c * (2.0f * mt * t) + p1 * (t * t);
            ctx->current->push_back(ctx->toWorld(pt));
        }

        ctx->lastPoint = p1;
        return 0;
    }

    static int outlineCubicTo(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user)
    {
        auto* ctx = static_cast<OutlineDecomposeContext*>(user);
        const float2 p0 = ctx->lastPoint;
        const float2 c1 = OutlineDecomposeContext::fromFixed(*control1);
        const float2 c2 = OutlineDecomposeContext::fromFixed(*control2);
        const float2 p1 = OutlineDecomposeContext::fromFixed(*to);

        for (int i = 1; i <= ctx->curveDetail; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(ctx->curveDetail);
            const float mt = 1.0f - t;
            const float2 pt = p0 * (mt * mt * mt) + c1 * (3.0f * mt * mt * t) + c2 * (3.0f * mt * t * t) + p1 * (t * t * t);
            ctx->current->push_back(ctx->toWorld(pt));
        }

        ctx->lastPoint = p1;
        return 0;
    }
} // namespace p5cpp

namespace p5cpp::detail
{
    class FontRasterizer
    {
    public:
        static std::unique_ptr<FontRasterizer> loadFromFile(const std::filesystem::path& fontFilePath)
        {
            FT_Face rawFace;
            FT_Error error = FT_New_Face(freetype.library, fontFilePath.string().c_str(), 0, &rawFace);
            if (error) {
                return nullptr;
            }

            FreetypeFace face(rawFace);
            return std::unique_ptr<FontRasterizer>(new FontRasterizer(std::move(face)));
        }

        static std::unique_ptr<FontRasterizer> loadFromMemory(std::span<const uint8_t> fontData)
        {
            FT_Face rawFace;
            FT_Error error = FT_New_Memory_Face(freetype.library, fontData.data(), static_cast<FT_Long>(fontData.size()), 0, &rawFace);
            if (error) {
                return nullptr;
            }

            FreetypeFace face(rawFace);
            return std::unique_ptr<FontRasterizer>(new FontRasterizer(std::move(face)));
        }

        const FontMetrics* getMetrics(int textSize)
        {
            if (const FontMetrics* cachedMetrics = m_glyphMetricsCache.get(textSize)) {
                return cachedMetrics;
            }

            if (FT_Set_Pixel_Sizes(m_face.get(), 0, static_cast<FT_UInt>(textSize))) {
                return nullptr;
            }

            const float ascender = m_face->size->metrics.ascender / 64.0f;   // Convert from 26.6 fixed point format to float.
            const float descender = m_face->size->metrics.descender / 64.0f; // Convert from 26.6 fixed point format to float.
            const float lineHeight = m_face->size->metrics.height / 64.0f;   // Convert from 26.6 fixed point format to float.

            FontMetrics metrics {
                .ascender = ascender,
                .descender = descender,
                .lineHeight = lineHeight,
            };

            return m_glyphMetricsCache.put(textSize, metrics);
        }

        size_t pageCount() const
        {
            return m_glyphAtlasPages.size();
        }

        const Texture* pageTexture(size_t index) const
        {
            return index < m_glyphAtlasPages.size() ? m_glyphAtlasPages[index]->getTexture() : nullptr;
        }

        std::vector<ShapedGlyph> shape(std::string_view text, int textSize)
        {
            if (const std::vector<ShapedGlyph>* cached = m_shapedTextCache.get(text, textSize)) {
                return *cached;
            }

            if (!m_hbFont) {
                m_hbFont.reset(hb_ft_font_create(m_face.get(), nullptr));
            }

            if (FT_Set_Pixel_Sizes(m_face.get(), 0, static_cast<FT_UInt>(textSize))) {
                return {};
            }

            hb_ft_font_changed(m_hbFont.get());

            hb_buffer_t* buf = hb_buffer_create();
            hb_buffer_add_utf8(buf, text.data(), static_cast<int>(text.size()), 0, static_cast<int>(text.size()));
            hb_buffer_guess_segment_properties(buf);

            hb_feature_t features[3] = {};
            hb_feature_from_string("liga", -1, &features[0]); // standard ligatures
            hb_feature_from_string("calt", -1, &features[1]); // contextual alternates
            hb_feature_from_string("kern", -1, &features[2]); // kerning

            hb_shape(m_hbFont.get(), buf, features, 3);

            unsigned int numGlyphs = 0;
            hb_glyph_info_t* glyphInfos = hb_buffer_get_glyph_infos(buf, &numGlyphs);
            hb_glyph_position_t* glyphPositions = hb_buffer_get_glyph_positions(buf, nullptr);

            std::vector<ShapedGlyph> result;
            result.reserve(numGlyphs);

            for (unsigned int i = 0; i < numGlyphs; ++i) {
                const uint32_t glyphId = glyphInfos[i].codepoint; // HarfBuzz renames this field to hold glyph ID
                const uint32_t clusterByteOffset = glyphInfos[i].cluster;

                const float xOffset = glyphPositions[i].x_offset / 64.0f;
                const float yOffset = glyphPositions[i].y_offset / 64.0f;
                const float xAdvance = glyphPositions[i].x_advance / 64.0f;
                const float yAdvance = glyphPositions[i].y_advance / 64.0f;

                const char32_t cluster = decodeUtf8CodepointAt(text, clusterByteOffset);
                const Glyph* glyph = renderGlyphById(glyphId, textSize);

                if (glyph == nullptr) {
                    result.push_back(ShapedGlyph {
                        .bearing = {0, 0},
                        .size = {0, 0},
                        .uvRect = {0.0f, 0.0f, 0.0f, 0.0f},
                        .glyphAtlasIndex = 0,
                        .xOffset = xOffset,
                        .yOffset = yOffset,
                        .xAdvance = xAdvance,
                        .yAdvance = yAdvance,
                        .isWhitespace = true,
                        .cluster = cluster,
                    });
                } else {
                    const bool hasNoPixels = (glyph->region.size.x == 0 || glyph->region.size.y == 0);
                    result.push_back(ShapedGlyph {
                        .bearing = glyph->bearing,
                        .size = glyph->region.size,
                        .uvRect = glyph->region.uvRect,
                        .glyphAtlasIndex = glyph->glyphAtlasIndex,
                        .xOffset = xOffset,
                        .yOffset = yOffset,
                        .xAdvance = xAdvance,
                        .yAdvance = yAdvance,
                        .isWhitespace = hasNoPixels,
                        .cluster = cluster,
                    });
                }
            }

            hb_buffer_destroy(buf);
            return *m_shapedTextCache.put(text, textSize, std::move(result));
        }

        std::vector<TextContour> textToPoints(std::string_view text, float x, float y, int textSize, int curveDetail, float maxWidth, TextWrap wrap)
        {
            std::vector<TextContour> result;

            if (!m_hbFont) {
                m_hbFont.reset(hb_ft_font_create(m_face.get(), nullptr));
            }

            if (FT_Set_Pixel_Sizes(m_face.get(), 0, static_cast<FT_UInt>(textSize))) {
                return result;
            }
            hb_ft_font_changed(m_hbFont.get());

            const FontMetrics* metrics = getMetrics(textSize);
            const float lineHeight = metrics ? metrics->lineHeight : static_cast<float>(textSize) * 1.2f;

            FT_Outline_Funcs funcs {};
            funcs.move_to = &outlineMoveTo;
            funcs.line_to = &outlineLineTo;
            funcs.conic_to = &outlineConicTo;
            funcs.cubic_to = &outlineCubicTo;
            funcs.shift = 0;
            funcs.delta = 0;

            hb_feature_t features[3] = {};
            hb_feature_from_string("liga", -1, &features[0]); // standard ligatures
            hb_feature_from_string("calt", -1, &features[1]); // contextual alternates
            hb_feature_from_string("kern", -1, &features[2]); // kerning

            const bool doWrap = (maxWidth > 0.0f) && (wrap != TextWrap::none);

            // A contiguous run of glyphs [start, end) making up one visual (post-wrap) line.
            struct GlyphRange
            {
                unsigned int start;
                unsigned int end;
            };

            float penY = y;
            size_t lineStart = 0;
            while (lineStart <= text.size()) {
                const size_t newlinePos = text.find('\n', lineStart);
                const std::string_view line = (newlinePos == std::string_view::npos)
                    ? text.substr(lineStart)
                    : text.substr(lineStart, newlinePos - lineStart);

                hb_buffer_t* buf = hb_buffer_create();
                hb_buffer_add_utf8(buf, line.data(), static_cast<int>(line.size()), 0, static_cast<int>(line.size()));
                hb_buffer_guess_segment_properties(buf);
                hb_shape(m_hbFont.get(), buf, features, 3);

                unsigned int numGlyphs = 0;
                hb_glyph_info_t* glyphInfos = hb_buffer_get_glyph_infos(buf, &numGlyphs);
                hb_glyph_position_t* glyphPositions = hb_buffer_get_glyph_positions(buf, nullptr);

                auto glyphAdvance = [&](unsigned int i) { return glyphPositions[i].x_advance / 64.0f; };
                auto isWhitespaceGlyph = [&](unsigned int i) {
                    const char32_t cp = decodeUtf8CodepointAt(line, glyphInfos[i].cluster);
                    return cp == U' ' || cp == U'\t';
                };

                // Split this paragraph's glyphs into visual sub-lines according to maxWidth/wrap,
                // mirroring the wrap rules of shapeTextLines() in graphics_component.cpp.
                std::vector<GlyphRange> visualLines;
                if (!doWrap || numGlyphs == 0) {
                    visualLines.push_back({0, numGlyphs});
                } else if (wrap == TextWrap::character) {
                    unsigned int rangeStart = 0;
                    float width = 0.0f;
                    for (unsigned int i = 0; i < numGlyphs; ++i) {
                        const float adv = glyphAdvance(i);
                        if (width + adv > maxWidth && i > rangeStart) {
                            visualLines.push_back({rangeStart, i});
                            rangeStart = i;
                            width = 0.0f;
                        }
                        width += adv;
                    }
                    visualLines.push_back({rangeStart, numGlyphs});
                } else {
                    // TextWrap::word — greedy fill at whitespace boundaries
                    unsigned int rangeStart = 0;
                    float width = 0.0f;
                    unsigned int lastWhitespace = numGlyphs; // sentinel: none seen yet in this range
                    for (unsigned int i = 0; i < numGlyphs; ++i) {
                        const float adv = glyphAdvance(i);
                        if (width + adv > maxWidth && i > rangeStart) {
                            if (lastWhitespace != numGlyphs && lastWhitespace >= rangeStart) {
                                visualLines.push_back({rangeStart, lastWhitespace});
                                rangeStart = lastWhitespace + 1;
                            } else {
                                visualLines.push_back({rangeStart, i});
                                rangeStart = i;
                            }
                            width = 0.0f;
                            for (unsigned int k = rangeStart; k <= i; ++k) width += glyphAdvance(k);
                            lastWhitespace = numGlyphs;
                        } else {
                            width += adv;
                        }
                        if (isWhitespaceGlyph(i)) lastWhitespace = i;
                    }
                    visualLines.push_back({rangeStart, numGlyphs});
                }

                for (size_t vi = 0; vi < visualLines.size(); ++vi) {
                    const GlyphRange range = visualLines[vi];
                    float penX = x;
                    for (unsigned int i = range.start; i < range.end; ++i) {
                        const uint32_t glyphId = glyphInfos[i].codepoint; // HarfBuzz renames this field to hold glyph ID
                        const float xOffset = glyphPositions[i].x_offset / 64.0f;
                        const float yOffset = glyphPositions[i].y_offset / 64.0f;
                        const float xAdvance = glyphPositions[i].x_advance / 64.0f;
                        const float yAdvance = glyphPositions[i].y_advance / 64.0f;

                        if (FT_Load_Glyph(m_face.get(), glyphId, FT_LOAD_NO_BITMAP) == 0) {
                            FT_Outline& outline = m_face->glyph->outline;
                            if (outline.n_points > 0) {
                                OutlineDecomposeContext ctx;
                                ctx.contours = &result;
                                ctx.penOffset = float2 {penX + xOffset, penY - yOffset};
                                ctx.curveDetail = curveDetail < 1 ? 1 : curveDetail;
                                FT_Outline_Decompose(&outline, &funcs, &ctx);
                            }
                        }

                        penX += xAdvance;
                        penY += yAdvance;
                    }

                    if (vi + 1 < visualLines.size()) {
                        penY += lineHeight;
                    }
                }

                hb_buffer_destroy(buf);

                if (newlinePos == std::string_view::npos) {
                    break;
                }
                lineStart = newlinePos + 1;
                penY += lineHeight;
            }

            return result;
        }

    private:
        explicit FontRasterizer(FreetypeFace face)
            : m_face(std::move(face))
        {
        }

        // Rasterise a glyph by its FreeType/HarfBuzz glyph ID and store it in the atlas.
        // Returns nullptr only if FreeType fails to load/render the glyph.
        const Glyph* renderGlyphById(uint32_t glyphId, int textSize)
        {
            if (const Glyph* cached = m_glyphByIdCache.get(glyphId, textSize)) {
                return cached;
            }

            if (FT_Load_Glyph(m_face.get(), glyphId, FT_LOAD_NO_BITMAP)) {
                return nullptr;
            }

            if (FT_Render_Glyph(m_face->glyph, FT_RENDER_MODE_NORMAL)) {
                return nullptr;
            }

            const int bearingX = m_face->glyph->bitmap_left;
            const int bearingY = m_face->glyph->bitmap_top;
            const int advanceX = m_face->glyph->advance.x / 64;

            const int bitmapWidth = static_cast<int>(m_face->glyph->bitmap.width);
            const int bitmapHeight = static_cast<int>(m_face->glyph->bitmap.rows);
            const std::span<const uint8_t> bitmapData(m_face->glyph->bitmap.buffer, bitmapWidth * bitmapHeight);

            const auto [region, pageIndex] = storeInAtlas(bitmapWidth, bitmapHeight, bitmapData);
            if (!region.has_value()) {
                return nullptr;
            }

            Glyph glyph {
                .region = region.value(),
                .bearing = int2 {bearingX, bearingY},
                .advanceX = static_cast<float>(advanceX),
                .glyphAtlasIndex = pageIndex,
            };

            return m_glyphByIdCache.put(glyphId, textSize, glyph);
        }

        struct AtlasStorageResult
        {
            std::optional<GlyphRegion> region;
            size_t pageIndex = 0;
        };

        AtlasStorageResult storeInAtlas(int bitmapWidth, int bitmapHeight, std::span<const uint8_t> bitmapData)
        {
            const bool needsToCreateInitialAtlas = m_glyphAtlasPages.empty();
            if (needsToCreateInitialAtlas) {
                m_glyphAtlasPages.emplace_back(std::make_unique<GlyphAtlas>(512, 512, 1, 1));
            }

            const size_t currentPageIndex = m_glyphAtlasPages.size() - 1;
            const auto region = m_glyphAtlasPages.back()->store(bitmapWidth, bitmapHeight, bitmapData);
            if (region.has_value()) {
                return {region, currentPageIndex};
            }

            if (needsToCreateInitialAtlas) {
                error("Failed to store the glyph in the atlas. The glyph might be too large to fit in the atlas.");
                return {std::nullopt, 0};
            }

            m_glyphAtlasPages.emplace_back(std::make_unique<GlyphAtlas>(512, 512, 1, 1));
            const size_t newPageIndex = m_glyphAtlasPages.size() - 1;
            return {m_glyphAtlasPages.back()->store(bitmapWidth, bitmapHeight, bitmapData), newPageIndex};
        }

        static char32_t decodeUtf8CodepointAt(std::string_view text, uint32_t byteOffset)
        {
            if (byteOffset >= text.size()) {
                return U' ';
            }

            const auto u = [&](uint32_t offset) -> uint8_t { return static_cast<uint8_t>(text[offset]); };
            const uint8_t first = u(byteOffset);

            if (first < 0x80) {
                return static_cast<char32_t>(first);
            }
            if ((first & 0xE0) == 0xC0 && byteOffset + 1 < text.size()) {
                return static_cast<char32_t>(((first & 0x1F) << 6) | (u(byteOffset + 1) & 0x3F));
            }
            if ((first & 0xF0) == 0xE0 && byteOffset + 2 < text.size()) {
                return static_cast<char32_t>(((first & 0x0F) << 12) | ((u(byteOffset + 1) & 0x3F) << 6) | (u(byteOffset + 2) & 0x3F));
            }
            if ((first & 0xF8) == 0xF0 && byteOffset + 3 < text.size()) {
                return static_cast<char32_t>(((first & 0x07) << 18) | ((u(byteOffset + 1) & 0x3F) << 12) | ((u(byteOffset + 2) & 0x3F) << 6) | (u(byteOffset + 3) & 0x3F));
            }

            return U' '; // Malformed byte — treat as space.
        }

        GlyphByIdCache m_glyphByIdCache;
        FontMetricsCache m_glyphMetricsCache;
        ShapedTextCache m_shapedTextCache;

        std::vector<std::unique_ptr<GlyphAtlas>> m_glyphAtlasPages;
        FreetypeFace m_face;
        HbFont m_hbFont;
    };
} // namespace p5cpp::detail

namespace p5cpp
{
    // Defined here, not inline in font.hpp: needs detail::FontRasterizer to be a
    // complete type to destroy the unique_ptr<FontRasterizer> member.
    FontData::~FontData() = default;
} // namespace p5cpp

namespace p5cpp
{
    // Resamples a closed contour to points spaced `spacing` pixels apart along its arc length
    // (walking through the implicit closing edge back to points[0] too). Leaves the contour
    // untouched if spacing is non-positive or it has too few points to measure a perimeter.
    static TextContour resampleContourEvenly(const TextContour& contour, float spacing)
    {
        if (spacing <= 0.0f || contour.size() < 2) {
            return contour;
        }

        const size_t n = contour.size();
        std::vector<float> edgeLengths(n);
        float perimeter = 0.0f;
        for (size_t i = 0; i < n; ++i) {
            edgeLengths[i] = length(contour[(i + 1) % n] - contour[i]);
            perimeter += edgeLengths[i];
        }

        if (perimeter <= 0.0f) {
            return contour;
        }

        size_t count = static_cast<size_t>(std::round(perimeter / spacing));
        if (count < 3) {
            count = 3;
        }

        TextContour result;
        result.reserve(count);

        size_t edgeIndex = 0;
        float edgeStart = 0.0f;
        float edgeEnd = edgeLengths[0];

        for (size_t i = 0; i < count; ++i) {
            const float targetDist = perimeter * (static_cast<float>(i) / static_cast<float>(count));

            while (targetDist > edgeEnd && edgeIndex + 1 < n) {
                ++edgeIndex;
                edgeStart = edgeEnd;
                edgeEnd += edgeLengths[edgeIndex];
            }

            const float edgeLen = edgeLengths[edgeIndex];
            const float t = edgeLen > 0.0f ? (targetDist - edgeStart) / edgeLen : 0.0f;
            result.push_back(lerp(contour[edgeIndex], contour[(edgeIndex + 1) % n], t));
        }

        return result;
    }
} // namespace p5cpp

namespace
{
    // loadFont(path) caches by file path: repeated calls for the same file while a
    // prior handle is still alive return a new lightweight Font aliasing the same
    // parsed FontData instead of re-running FT_New_Face (see CLAUDE.md's "Resource
    // loading" section).
    std::unordered_map<std::string, std::weak_ptr<p5cpp::FontData>> s_fontCache;

    // Appends any glyph atlas pages the rasterizer created since the last call (see
    // FontData::glyphAtlasPages in font.hpp) - called after shape()/textToPoints(),
    // the only two operations that can rasterize a glyph that doesn't fit yet.
    void syncGlyphAtlasPages(p5cpp::FontData& data)
    {
        while (data.glyphAtlasPages.size() < data.rasterizer->pageCount()) {
            data.glyphAtlasPages.push_back(*data.rasterizer->pageTexture(data.glyphAtlasPages.size()));
        }
    }
} // namespace

namespace p5cpp
{
    Font loadFont(const std::filesystem::path& fontFilePath)
    {
        const std::string key = fontFilePath.string();

        if (const auto it = s_fontCache.find(key); it != s_fontCache.end()) {
            if (std::shared_ptr<FontData> cached = it->second.lock()) {
                return Font {std::move(cached)};
            }
        }

        std::unique_ptr<detail::FontRasterizer> rasterizer = detail::FontRasterizer::loadFromFile(fontFilePath);
        if (!rasterizer) {
            return Font();
        }

        auto data = std::make_shared<FontData>();
        data->rasterizer = std::move(rasterizer);

        s_fontCache[key] = data;
        return Font {std::move(data)};
    }

    Font loadFont(std::span<const uint8_t> fontData)
    {
        std::unique_ptr<detail::FontRasterizer> rasterizer = detail::FontRasterizer::loadFromMemory(fontData);
        if (!rasterizer) {
            return Font();
        }

        auto data = std::make_shared<FontData>();
        data->rasterizer = std::move(rasterizer);

        return Font {std::move(data)};
    }

    bool isFontValid(const Font& font)
    {
        return font.data != nullptr;
    }

    const FontMetrics* getMetrics(const Font& font, int textSize)
    {
        assert(font.data != nullptr && "Font is not initialized.");
        return font.data->rasterizer->getMetrics(textSize);
    }

    const Texture* getGlyphAtlasTexture(const Font& font, size_t glyphAtlasIndex)
    {
        assert(font.data != nullptr && "Font is not initialized.");
        if (glyphAtlasIndex >= font.data->glyphAtlasPages.size()) {
            return nullptr;
        }
        return &font.data->glyphAtlasPages[glyphAtlasIndex];
    }

    std::vector<ShapedGlyph> shape(const Font& font, std::string_view text, int textSize)
    {
        assert(font.data != nullptr && "Font is not initialized.");
        std::vector<ShapedGlyph> result = font.data->rasterizer->shape(text, textSize);
        syncGlyphAtlasPages(*font.data);
        return result;
    }

    std::vector<TextContour> textToPoints(const Font& font, std::string_view text, float x, float y, int textSize, int curveDetail, float spacing, float maxWidth, TextWrap wrap)
    {
        assert(font.data != nullptr && "Font is not initialized.");
        std::vector<TextContour> contours = font.data->rasterizer->textToPoints(text, x, y, textSize, curveDetail, maxWidth, wrap);
        syncGlyphAtlasPages(*font.data);

        if (spacing > 0.0f) {
            for (TextContour& contour : contours) {
                contour = resampleContourEvenly(contour, spacing);
            }
        }

        return contours;
    }
} // namespace p5cpp
