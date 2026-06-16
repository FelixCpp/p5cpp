#include <p5cpp.hpp>

#include <optional>
#include <algorithm>

#include <glad/glad.h>

#include <freetype/freetype.h>
#include <unordered_map>

namespace p5cpp
{
    struct BinPackingStrategy
    {
        virtual ~BinPackingStrategy() = default;
        virtual std::optional<rect2i> insert(int width, int height) = 0;
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

        std::optional<rect2i> insert(int width, int height) override
        {
            if (width <= 0 || height <= 0) {
                return rect2i {
                    .left = 0,
                    .top = 0,
                    .width = width,
                    .height = height,
                };
            }

            const std::optional<rect2i> placed = findBestShortSideFit(width, height);
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
        std::optional<rect2i> findBestShortSideFit(int width, int height) const
        {
            std::optional<rect2i> bestRect;
            int bestShortSide = std::numeric_limits<int>::max();
            int bestLongSide = std::numeric_limits<int>::max();

            for (const rect2i& freeRect : m_freeRects) {
                if (freeRect.width >= width && freeRect.height >= height) {
                    const int leftoverShort = std::min(freeRect.width - width, freeRect.height - height);
                    const int leftoverLong = std::max(freeRect.width - width, freeRect.height - height);

                    if (leftoverShort < bestShortSide ||
                        (leftoverShort == bestShortSide && leftoverLong < bestLongSide)) {
                        bestRect = rect2i {.left = freeRect.left, .top = freeRect.top, .width = width, .height = height};
                        bestShortSide = leftoverShort;
                        bestLongSide = leftoverLong;
                    }
                }
            }

            return bestRect;
        }

        // After placing a rectangle, split all free rectangles that overlap with it.
        void splitFreeRects(const rect2i& placed)
        {
            std::vector<rect2i> newFreeRects;

            for (int i = 0; i < static_cast<int>(m_freeRects.size()); ++i) {
                if (!overlaps(m_freeRects[i], placed)) {
                    continue;
                }

                const rect2i& free = m_freeRects[i];

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

            for (rect2i& r : newFreeRects) {
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

        static bool overlaps(const rect2i& a, const rect2i& b)
        {
            return a.left < b.left + b.width &&
                   a.left + a.width > b.left &&
                   a.top < b.top + b.height &&
                   a.top + a.height > b.top;
        }

        // Returns true if 'outer' ful.top contains 'inner'.
        static bool contains(const rect2i& outer, const rect2i& inner)
        {
            return inner.left >= outer.left &&
                   inner.top >= outer.top &&
                   inner.left + inner.width <= outer.left + outer.width &&
                   inner.top + inner.height <= outer.top + outer.height;
        }

        int m_binWidth;
        int m_binHeight;
        std::vector<rect2i> m_freeRects;
    };
} // namespace p5cpp

namespace p5cpp
{
    class GlyphAtlas
    {
    public:
        explicit GlyphAtlas(int width, int height, int paddingX, int paddingY)
            : m_width(width), m_height(height), m_paddingX(paddingX), m_paddingY(paddingY), m_textureId(0), m_packingStrategy(std::make_unique<MaxRectsBinPacking>(width, height))
        {
            glGenTextures(1, &m_textureId);
            glBindTexture(GL_TEXTURE_2D, m_textureId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            constexpr GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_RED};
            glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
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

            const std::optional<rect2i> placed = m_packingStrategy->insert(bitmapWidth + m_paddingX, bitmapHeight + m_paddingY);
            if (not placed.has_value()) {
                return std::nullopt;
            }

            glBindTexture(GL_TEXTURE_2D, m_textureId);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexSubImage2D(GL_TEXTURE_2D, 0, placed->left, placed->top, static_cast<GLsizei>(bitmapWidth), static_cast<GLsizei>(bitmapHeight), GL_RED, GL_UNSIGNED_BYTE, bitmapData.data());

            const float uvLeft = static_cast<float>(placed->left) / static_cast<float>(m_width);
            const float uvTop = static_cast<float>(placed->top) / static_cast<float>(m_height);
            const float uvRight = static_cast<float>(placed->left + bitmapWidth) / static_cast<float>(m_width);
            const float uvBottom = static_cast<float>(placed->top + bitmapHeight) / static_cast<float>(m_height);

            return GlyphRegion {
                .size = int2 {bitmapWidth, bitmapHeight},
                .uvRect = {
                    .left = uvLeft,
                    .top = uvTop,
                    .width = uvRight - uvLeft,
                    .height = uvBottom - uvTop,
                },
            };
        }

        GLuint getTextureId() const
        {
            return m_textureId;
        }

    private:
        int m_width;
        int m_height;
        int m_paddingX;
        int m_paddingY;
        GLuint m_textureId;

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
    class FreetypeFont : public Font
    {
    public:
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

            const int bitmapWidth = m_face->glyph->bitmap.width;
            const int bitmapHeight = m_face->glyph->bitmap.rows;

            const std::span<const uint8_t> bitmapData(m_face->glyph->bitmap.buffer, bitmapWidth * bitmapHeight);

            const auto region = std::invoke([&]() -> std::optional<GlyphRegion> {
                const bool needsToCreateInitialAtlas = m_glyphAtlasPages.empty();
                if (needsToCreateInitialAtlas) {
                    m_glyphAtlasPages.emplace_back(std::make_unique<GlyphAtlas>(512, 512, 1, 1));
                }

                const auto& currentGlyphAtlas = m_glyphAtlasPages.back();
                const auto region = currentGlyphAtlas->store(bitmapWidth, bitmapHeight, bitmapData);
                if (region.has_value()) {
                    return region;
                }

                if (needsToCreateInitialAtlas) {
                    error("Failed to store the glyph in the atlas. The glyph might be too large to fit in the atlas.");
                    return std::nullopt; // Failed to store the glyph in the atlas. Even though the atlas is empty, the glyph might be too large to fit in the atlas.
                }

                auto& newAtlas = m_glyphAtlasPages.emplace_back(std::make_unique<GlyphAtlas>(512, 512, 1, 1));
                return newAtlas->store(bitmapWidth, bitmapHeight, bitmapData);
            });

            Glyph glyph {
                .region = region.value(),
                .bearing = int2 {bearingX, bearingY},
                .advanceX = static_cast<float>(advanceX),
                .glyphAtlasIndex = 0,
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

        uint32_t getGlyphAtlasTextureId(size_t glyphAtlasIndex) override
        {
            if (glyphAtlasIndex >= m_glyphAtlasPages.size()) {
                return 0; // Invalid atlas index.
            }

            return m_glyphAtlasPages[glyphAtlasIndex]->getTextureId();
        }

    private:
        explicit FreetypeFont(FreetypeFace face)
            : m_face(std::move(face))
        {
        }

        KerningCache m_kerningCache;
        GlyphCache m_glyphCache;
        FontMetricsCache m_glyphMetricsCache;

        std::vector<std::unique_ptr<GlyphAtlas>> m_glyphAtlasPages;
        FreetypeFace m_face;
    };
} // namespace p5cpp

namespace p5cpp
{
    std::unique_ptr<Font> loadFont(const std::filesystem::path& fontFilePath)
    {
        return nullptr;
    }

    std::unique_ptr<Font> loadFont(std::span<const uint8_t> fontData)
    {
        return FreetypeFont::loadFromMemory(fontData);
    }
} // namespace p5cpp
