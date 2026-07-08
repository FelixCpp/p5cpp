#include <p5cpp/graphics/font.hpp>
#include <p5cpp/application/logging.hpp>

#include <glad/glad.h>
#include <freetype/freetype.h>
#include <hb.h>
#include <hb-ft.h>

#include <unordered_map>
#include <optional>
#include <vector>
#include <string>
#include <string_view>
#include <cassert>

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
    class GlyphAtlasTexture : public TextureImpl
    {
    public:
        static std::unique_ptr<GlyphAtlasTexture> create(int width, int height)
        {
            GLuint textureId = 0;
            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            constexpr GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_RED};
            glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

            return std::unique_ptr<GlyphAtlasTexture>(new GlyphAtlasTexture(textureId, width, height));
        }

        uint2 getSize() const override
        {
            return uint2 {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        }

        TextureId getTextureId() const override
        {
            return TextureId {.value = textureId};
        }

        void upload(std::span<const color_t> data) override
        {
            throw std::runtime_error("GlyphAtlasTexture does not support updating the entire texture.");
        }

        void store(int x, int y, int width, int height, std::span<const uint8_t> bitmapData)
        {
            glBindTexture(GL_TEXTURE_2D, textureId);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RED, GL_UNSIGNED_BYTE, bitmapData.data());
        }

    private:
        explicit GlyphAtlasTexture(GLuint textureId, int width, int height)
            : textureId(textureId), width(width), height(height)
        {
        }

        GLuint textureId;
        int width, height;
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
              m_atlasTexture(GlyphAtlasTexture::create(width, height)),
              m_texture(m_atlasTexture),
              m_packingStrategy(std::make_unique<MaxRectsBinPacking>(width, height))
        {
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

            m_atlasTexture->store(placed->left, placed->top, bitmapWidth, bitmapHeight, bitmapData);

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

        std::shared_ptr<GlyphAtlasTexture> m_atlasTexture;
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
    struct KerningCacheKey
    {
        char32_t leftCodepoint;
        char32_t rightCodepoint;
        int textSize;

        bool operator==(const KerningCacheKey& other) const
        {
            return leftCodepoint == other.leftCodepoint && rightCodepoint == other.rightCodepoint && textSize == other.textSize;
        }
    };

    struct KerningCacheKeyHasher
    {
        std::size_t operator()(const KerningCacheKey& key) const
        {
            std::size_t h1 = std::hash<char32_t> {}(key.leftCodepoint);
            std::size_t h2 = std::hash<char32_t> {}(key.rightCodepoint);
            std::size_t h3 = std::hash<int> {}(key.textSize);
            return h1 ^ (h2 << 1) ^ (h3 << 2); // Combine the hash values of the members.
        }
    };

    class KerningCache
    {
    public:
        std::optional<float> get(char32_t leftCodepoint, char32_t rightCodepoint, int textSize) const
        {
            const KerningCacheKey key {
                .leftCodepoint = leftCodepoint,
                .rightCodepoint = rightCodepoint,
                .textSize = textSize,
            };

            const auto itr = m_cache.find(key);
            if (itr != m_cache.end()) {
                return itr->second;
            }

            return std::nullopt;
        }

        void put(char32_t leftCodepoint, char32_t rightCodepoint, int textSize, float kerning)
        {
            KerningCacheKey key {leftCodepoint, rightCodepoint, textSize};
            m_cache.insert(std::make_pair(key, kerning));
        }

    private:
        std::unordered_map<KerningCacheKey, float, KerningCacheKeyHasher> m_cache;
    };
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
    struct GlyphCacheKey
    {
        char32_t codepoint;
        int textSize;

        bool operator==(const GlyphCacheKey& other) const
        {
            return codepoint == other.codepoint && textSize == other.textSize;
        }
    };

    struct GlyphCacheKeyHasher
    {
        std::size_t operator()(const GlyphCacheKey& key) const
        {
            std::size_t h1 = std::hash<char32_t> {}(key.codepoint);
            std::size_t h2 = std::hash<int> {}(key.textSize);
            return h1 ^ (h2 << 1); // Combine the hash values of the members.
        }
    };

    class GlyphCache
    {
    public:
        const Glyph* get(char32_t codepoint, int textSize) const
        {
            const GlyphCacheKey key {
                .codepoint = codepoint,
                .textSize = textSize,
            };

            const auto itr = m_cache.find(key);
            if (itr != m_cache.end()) {
                return &itr->second;
            }

            return nullptr;
        }

        const Glyph* put(char32_t codepoint, int textSize, const Glyph& glyph)
        {
            GlyphCacheKey key {codepoint, textSize};
            const auto insertion = m_cache.emplace(std::make_pair(key, glyph));
            return &insertion.first->second;
        }

    private:
        std::unordered_map<GlyphCacheKey, Glyph, GlyphCacheKeyHasher> m_cache;
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
    class FreetypeFont : public FontImpl
    {
    public:
        static std::unique_ptr<FreetypeFont> loadFromFile(const std::filesystem::path& fontFilePath)
        {
            FT_Face rawFace;
            FT_Error error = FT_New_Face(freetype.library, fontFilePath.string().c_str(), 0, &rawFace);
            if (error) {
                return nullptr;
            }

            FreetypeFace face(rawFace);
            return std::unique_ptr<FreetypeFont>(new FreetypeFont(std::move(face)));
        }

        static std::unique_ptr<FreetypeFont> loadFromMemory(std::span<const uint8_t> fontData)
        {
            FT_Face rawFace;
            FT_Error error = FT_New_Memory_Face(freetype.library, fontData.data(), static_cast<FT_Long>(fontData.size()), 0, &rawFace);
            if (error) {
                return nullptr;
            }

            FreetypeFace face(rawFace);
            return std::unique_ptr<FreetypeFont>(new FreetypeFont(std::move(face)));
        }

        const Glyph* getGlyph(char32_t codepoint, int textSize) override
        {
            if (const Glyph* cachedGlyph = m_glyphCache.get(codepoint, textSize)) {
                return cachedGlyph;
            }

            if (FT_Set_Pixel_Sizes(m_face.get(), 0, static_cast<FT_UInt>(textSize))) {
                return nullptr;
            }

            const FT_UInt glyphIndex = FT_Get_Char_Index(m_face.get(), codepoint);
            if (glyphIndex == 0) {
                return nullptr; // The font doesn't contain a glyph for the given codepoint.
            }

            if (FT_Load_Glyph(m_face.get(), glyphIndex, FT_LOAD_NO_BITMAP)) {
                return nullptr; // Failed to load the glyph.
            }

            if (FT_Render_Glyph(m_face->glyph, FT_RENDER_MODE_NORMAL)) {
                return nullptr; // Failed to render the glyph.
            }

            const int bearingX = m_face->glyph->bitmap_left;
            const int bearingY = m_face->glyph->bitmap_top;
            const int advanceX = m_face->glyph->advance.x / 64;

            const int bitmapWidth = static_cast<int>(m_face->glyph->bitmap.width);
            const int bitmapHeight = static_cast<int>(m_face->glyph->bitmap.rows);
            const std::span<const uint8_t> bitmapData(m_face->glyph->bitmap.buffer, bitmapWidth * bitmapHeight);

            const auto [region, atlasPageIndex] = storeInAtlas(bitmapWidth, bitmapHeight, bitmapData);
            if (!region.has_value()) {
                return nullptr;
            }

            Glyph glyph {
                .region = region.value(),
                .bearing = int2 {bearingX, bearingY},
                .advanceX = static_cast<float>(advanceX),
                .glyphAtlasIndex = atlasPageIndex,
            };

            return m_glyphCache.put(codepoint, textSize, glyph);
        }

        const FontMetrics* getMetrics(int textSize) override
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

        float getKerning(char32_t leftCodepoint, char32_t rightCodepoint, int textSize) override
        {
            const std::optional<float> cachedKerning = m_kerningCache.get(leftCodepoint, rightCodepoint, textSize);
            if (cachedKerning.has_value()) {
                return cachedKerning.value();
            }

            if (FT_Set_Pixel_Sizes(m_face.get(), 0, static_cast<FT_UInt>(textSize))) {
                return 0.0f;
            }

            const FT_UInt leftGlyphIndex = FT_Get_Char_Index(m_face.get(), leftCodepoint);
            const FT_UInt rightGlyphIndex = FT_Get_Char_Index(m_face.get(), rightCodepoint);

            FT_Vector kerning;
            if (FT_Get_Kerning(m_face.get(), leftGlyphIndex, rightGlyphIndex, FT_KERNING_DEFAULT, &kerning)) {
                return 0.0f;
            }

            const float kerningValue = kerning.x / 64.0f; // Convert from 26.6 fixed point format to float.
            m_kerningCache.put(leftCodepoint, rightCodepoint, textSize, kerningValue);
            return kerningValue;
        }

        const Texture* getGlyphAtlasTexture(size_t glyphAtlasIndex) override
        {
            if (glyphAtlasIndex >= m_glyphAtlasPages.size()) {
                return 0; // Invalid atlas index.
            }

            return m_glyphAtlasPages[glyphAtlasIndex]->getTexture();
        }

        std::vector<ShapedGlyph> shape(std::string_view text, int textSize) override
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

    private:
        explicit FreetypeFont(FreetypeFace face)
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

        KerningCache m_kerningCache;
        GlyphCache m_glyphCache;
        GlyphByIdCache m_glyphByIdCache;
        FontMetricsCache m_glyphMetricsCache;
        ShapedTextCache m_shapedTextCache;

        std::vector<std::unique_ptr<GlyphAtlas>> m_glyphAtlasPages;
        FreetypeFace m_face;
        HbFont m_hbFont;
    };
} // namespace p5cpp

namespace p5cpp
{
    std::unique_ptr<FontImpl> loadFont(const std::filesystem::path& fontFilePath)
    {
        return FreetypeFont::loadFromFile(fontFilePath);
    }

    std::unique_ptr<FontImpl> loadFont(std::span<const uint8_t> fontData)
    {
        return FreetypeFont::loadFromMemory(fontData);
    }
} // namespace p5cpp

namespace p5cpp
{
    Font::Font()
        : impl(nullptr)
    {
    }

    Font::Font(std::unique_ptr<FontImpl> impl)
        : impl(std::move(impl))
    {
    }

    Font::Font(std::shared_ptr<FontImpl> impl)
        : impl(std::move(impl))
    {
    }

    const Glyph* Font::getGlyph(char32_t codepoint, int textSize) const
    {
        assert(impl != nullptr && "Font implementation is not initialized.");
        return impl->getGlyph(codepoint, textSize);
    }

    const FontMetrics* Font::getMetrics(int textSize) const
    {
        assert(impl != nullptr && "Font implementation is not initialized.");
        return impl->getMetrics(textSize);
    }

    float Font::getKerning(char32_t leftCodepoint, char32_t rightCodepoint, int textSize) const
    {
        assert(impl != nullptr && "Font implementation is not initialized.");
        return impl->getKerning(leftCodepoint, rightCodepoint, textSize);
    }

    const Texture* Font::getGlyphAtlasTexture(size_t glyphAtlasIndex) const
    {
        assert(impl != nullptr && "Font implementation is not initialized.");
        return impl->getGlyphAtlasTexture(glyphAtlasIndex);
    }

    std::vector<ShapedGlyph> Font::shape(std::string_view text, int textSize) const
    {
        assert(impl != nullptr && "Font implementation is not initialized.");
        return impl->shape(text, textSize);
    }
} // namespace p5cpp
