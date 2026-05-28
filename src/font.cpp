#include "p5.hpp"
#include <freetype/freetype.h>
#include <freetype/ftstroke.h>
#include <unordered_map>
#include <vector>
#include <glad/glad.h>

namespace p5
{
    struct GlyphCacheEntry
    {
        char32_t codepoint; // Unicode codepoint of the glyph
        int textSize;       // Text size in pixels for which the glyph was loaded
        int strokeWeight;   // Stroke weight in pixels for which the glyph was loaded

        constexpr bool operator==(const GlyphCacheEntry& other) const = default;
        constexpr bool operator!=(const GlyphCacheEntry& other) const = default;
    };

    struct GlyphCacheEntryHasher
    {
        size_t operator()(const GlyphCacheEntry& entry) const
        {
            size_t hash = std::hash<char32_t> {}(entry.codepoint);
            hash ^= std::hash<int> {}(entry.textSize) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<int> {}(entry.strokeWeight) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            return hash;
        }
    };

    struct KerningCacheEntry
    {
        char32_t leftCodepoint;
        char32_t rightCodepoint;
        int textSize;

        constexpr bool operator==(const KerningCacheEntry& other) const = default;
        constexpr bool operator!=(const KerningCacheEntry& other) const = default;
    };

    struct KerningCacheEntryHasher
    {
        size_t operator()(const KerningCacheEntry& entry) const
        {
            size_t hash = std::hash<char32_t> {}(entry.leftCodepoint);
            hash ^= std::hash<char32_t> {}(entry.rightCodepoint) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<int> {}(entry.textSize) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            return hash;
        }
    };

    class FreeTypeFont : public Font
    {
    public:
        struct GlyphPage
        {
            uint2 size;             // width & height of the page in pixels
            uint2 glyphPadding;     // padding in pixels to add around each glyph to prevent texture bleeding
            uint32_t cursorX;       // horizontal position of the next glyph in the page
            uint32_t cursorY;       // vertical position of the next glyph in the page
            uint32_t lastRowHeight; // height of the tallest glyph in the current row in order to know how much to advance cursorY when moving to the next row

            GLuint textureId; // OpenGL texture ID of the page
        };

        static std::unique_ptr<Font> create(const std::filesystem::path& fontFilePath)
        {
            auto library = createLibrary();
            if (library == nullptr) {
                return nullptr;
            }

            auto face = createFace(library.get(), fontFilePath);
            if (face == nullptr) {
                return nullptr;
            }

            return std::unique_ptr<FreeTypeFont>(new FreeTypeFont(std::move(library), std::move(face)));
        }

        static std::unique_ptr<Font> create(std::span<const uint8_t> fontData)
        {
            auto library = createLibrary();
            if (library == nullptr) {
                return nullptr;
            }

            auto face = createFace(library.get(), fontData);
            if (face == nullptr) {
                return nullptr;
            }

            return std::unique_ptr<FreeTypeFont>(new FreeTypeFont(std::move(library), std::move(face)));
        }

        ~FreeTypeFont() override
        {
            for (const GlyphPage& page : glyphPages) {
                glDeleteTextures(1, &page.textureId);
            }
        }

        uint32_t getGlyphPageTextureId(GlyphPageIndex index) override
        {
            if (index >= glyphPages.size()) {
                throw std::out_of_range("Invalid glyph page index");
            }

            return glyphPages[index].textureId;
        }

        const Glyph* getGlyph(char32_t codepoint, int textSize) override
        {
            const auto cacheEntry = GlyphCacheEntry {
                .codepoint = codepoint,
                .textSize = textSize,
                .strokeWeight = 0,
            };

            const auto itr = glyphs.find(cacheEntry);
            if (itr != glyphs.end()) {
                return &itr->second;
            }

            if (FT_Set_Pixel_Sizes(face.get(), 0, textSize)) {
                return nullptr;
            }

            const FT_UInt glyphIndex = FT_Get_Char_Index(face.get(), codepoint);
            if (glyphIndex == 0) {
                return nullptr;
            }

            const FT_Error loadError = FT_Load_Glyph(face.get(), glyphIndex, FT_LOAD_NO_BITMAP);
            if (loadError) {
                return nullptr;
            }

            if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != 0) {
                return nullptr;
            }

            return storeGlyphInCache(
                cacheEntry,
                face->glyph->bitmap,
                face->glyph->bitmap_left,
                face->glyph->bitmap_top,
                face->glyph->advance.x
            );
        }

        float getLineHeight(int textSize) const override
        {
            const auto it = lineHeightCache.find(textSize);
            if (it != lineHeightCache.end()) {
                return it->second;
            }

            FT_Set_Pixel_Sizes(face.get(), 0, textSize);
            const float lineHeight = static_cast<float>(face->size->metrics.height) / 64.0f;
            lineHeightCache.emplace(textSize, lineHeight);
            return lineHeight;
        }

        float getKerning(char32_t leftCodepoint, char32_t rightCodepoint, int textSize) override
        {
            if (FT_HAS_KERNING(face.get()) == 0) {
                return 0.0f;
            }

            const KerningCacheEntry cacheEntry {
                .leftCodepoint = leftCodepoint,
                .rightCodepoint = rightCodepoint,
                .textSize = textSize,
            };

            const auto itr = kerningCache.find(cacheEntry);
            if (itr != kerningCache.end()) {
                return itr->second;
            }

            const float kerningValue = queryKerningFromFreeType(leftCodepoint, rightCodepoint, textSize);
            kerningCache.insert(std::make_pair(cacheEntry, kerningValue));
            return kerningValue;
        }

    private:
        float queryKerningFromFreeType(char32_t leftCodepoint, char32_t rightCodepoint, int textSize) const
        {
            const FT_UInt leftGlyphIndex = FT_Get_Char_Index(face.get(), leftCodepoint);
            const FT_UInt rightGlyphIndex = FT_Get_Char_Index(face.get(), rightCodepoint);
            if (leftGlyphIndex == 0 || rightGlyphIndex == 0) {
                return 0.0f;
            }

            FT_Vector kerning;
            if (FT_Get_Kerning(face.get(), leftGlyphIndex, rightGlyphIndex, FT_KERNING_DEFAULT, &kerning) != 0) {
                return 0.0f;
            }

            return static_cast<float>(kerning.x) / 64.0f; // FreeType uses 1/64th of a pixel as its unit for kerning
        }

        GlyphPage& getOrCreateGlyphPageForGlyph(const uint2& glyphSize)
        {
            if (glyphPages.empty() or not canFitGlyphInPage(glyphSize, glyphPages.back())) {
                static const uint2 glyphPageSize = {512, 512}; // TODO(Felix): Make this configurable and maybe support multiple page sizes

                // Create a new glyph page
                GlyphPage newPage {
                    .size = glyphPageSize,
                    .glyphPadding = {2, 2},
                    .cursorX = 0,
                    .cursorY = 0,
                    .lastRowHeight = 0,
                    .textureId = createNewGlyphPageTexture(glyphPageSize),
                };

                return glyphPages.emplace_back(newPage);
            }

            return glyphPages.back();
        }

        bool canFitGlyphInPage(const uint2 glyphSize, const GlyphPage& page) const
        {
            const uint32_t paddedWidth = static_cast<uint32_t>(glyphSize.x) + page.glyphPadding.x * 2;
            const uint32_t paddedHeight = static_cast<uint32_t>(glyphSize.y) + page.glyphPadding.y * 2;

            if (page.cursorX + paddedWidth > page.size.x) {
                // Not enough horizontal space in the current row, try moving to the next row
                if (page.cursorY + page.lastRowHeight + paddedHeight > page.size.y) {
                    return false; // Not enough vertical space in the page
                }
            } else {
                if (page.cursorY + paddedHeight > page.size.y) {
                    return false; // Not enough vertical space in the page
                }
            }

            return true;
        }

        rect2f putGlyphBitmapInPage(const uint8_t* glyphBitmapData, uint2 glyphSize, GlyphPage& page)
        {
            const uint32_t paddedWidth = static_cast<uint32_t>(glyphSize.x) + page.glyphPadding.x * 2;
            const uint32_t paddedHeight = static_cast<uint32_t>(glyphSize.y) + page.glyphPadding.y * 2;

            if (page.cursorX + paddedWidth > page.size.x) {
                // Move to the next row
                page.cursorX = 0;
                page.cursorY += page.lastRowHeight;
                page.lastRowHeight = 0;
            }

            const uint32_t x = page.cursorX + page.glyphPadding.x;
            const uint32_t y = page.cursorY + page.glyphPadding.y;

            // Upload the glyph bitmap to the GPU at the calculated position
            glBindTexture(GL_TEXTURE_2D, page.textureId);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, static_cast<GLsizei>(glyphSize.x), static_cast<GLsizei>(glyphSize.y), GL_RED, GL_UNSIGNED_BYTE, glyphBitmapData);

            // Update the glyph page cursor and last row height
            page.cursorX += paddedWidth;
            page.lastRowHeight = std::max(page.lastRowHeight, paddedHeight);

            const float2 uv0 = {static_cast<float>(x) / page.size.x, static_cast<float>(y) / page.size.y};
            const float2 uv1 = {static_cast<float>(x + glyphSize.x) / page.size.x, static_cast<float>(y + glyphSize.y) / page.size.y};

            return rect2f {
                .left = uv0.x,
                .top = uv0.y,
                .width = uv1.x - uv0.x,
                .height = uv1.y - uv0.y,
            };
        }

        const Glyph* storeGlyphInCache(const GlyphCacheEntry& cacheEntry, const FT_Bitmap& bitmap, int bitmapLeft, int bitmapTop, FT_Pos advanceX)
        {
            const uint2 glyphSize = {static_cast<uint32_t>(bitmap.width), static_cast<uint32_t>(bitmap.rows)};
            GlyphPage& glyphPage = getOrCreateGlyphPageForGlyph(glyphSize);
            const rect2f uvRect = putGlyphBitmapInPage(bitmap.buffer, glyphSize, glyphPage);

            const Glyph glyph = Glyph {
                .size = {static_cast<float>(glyphSize.x), static_cast<float>(glyphSize.y)},
                .bearing = {static_cast<float>(bitmapLeft), static_cast<float>(bitmapTop)},
                .uvRect = uvRect,
                .advance = {static_cast<float>(advanceX) / 64.0f, 0.0f}, // FreeType uses 1/64th of a pixel as its unit for advance
                .pageIndex = static_cast<GlyphPageIndex>(glyphPages.size() - 1),
            };

            return &glyphs.insert({cacheEntry, glyph}).first->second;
        }

        struct FT_Deleter
        {
            void operator()(FT_Library library) const
            {
                if (library) {
                    FT_Done_FreeType(library);
                }
            }
            void operator()(FT_Face face) const
            {
                if (face) {
                    FT_Done_Face(face);
                }
            }
            void operator()(FT_Stroker stroker) const
            {
                if (stroker) {
                    FT_Stroker_Done(stroker);
                }
            }
            void operator()(FT_Glyph glyph) const
            {
                if (glyph) {
                    FT_Done_Glyph(glyph);
                }
            }
        };

        explicit FreeTypeFont(std::unique_ptr<FT_LibraryRec_, FT_Deleter> library, std::unique_ptr<FT_FaceRec_, FT_Deleter> face)
            : library(std::move(library)), face(std::move(face))
        {
        }

        static GLuint createNewGlyphPageTexture(const uint2& size)
        {
            GLuint textureId;
            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, size.x, size.y, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            return textureId;
        }

        static std::unique_ptr<FT_LibraryRec_, FT_Deleter> createLibrary()
        {
            FT_Library library;
            if (FT_Init_FreeType(&library)) {
                return nullptr;
            }

            return std::unique_ptr<FT_LibraryRec_, FT_Deleter>(library);
        }

        static std::unique_ptr<FT_FaceRec_, FT_Deleter> createFace(FT_Library library, const std::filesystem::path& fontFilePath)
        {
            FT_Face face;
            if (FT_New_Face(library, fontFilePath.string().c_str(), 0, &face)) {
                return nullptr;
            }

            return std::unique_ptr<FT_FaceRec_, FT_Deleter>(face);
        }

        static std::unique_ptr<FT_FaceRec_, FT_Deleter> createFace(FT_Library library, std::span<const uint8_t> fontData)
        {
            FT_Face face;
            if (FT_New_Memory_Face(library, fontData.data(), static_cast<FT_Long>(fontData.size()), 0, &face)) {
                return nullptr;
            }

            return std::unique_ptr<FT_FaceRec_, FT_Deleter>(face);
        }

        std::unique_ptr<FT_LibraryRec_, FT_Deleter> library;
        std::unique_ptr<FT_FaceRec_, FT_Deleter> face;
        std::vector<GlyphPage> glyphPages;
        std::unordered_map<GlyphCacheEntry, Glyph, GlyphCacheEntryHasher> glyphs;
        std::unordered_map<KerningCacheEntry, float, KerningCacheEntryHasher> kerningCache;
        mutable std::unordered_map<int, float> lineHeightCache;
    };
} // namespace p5

namespace p5
{
    std::unique_ptr<Font> loadFont(const std::filesystem::path& fontFilePath)
    {
        return FreeTypeFont::create(fontFilePath);
    }

    std::unique_ptr<Font> loadFont(std::span<const uint8_t> fontData)
    {
        return FreeTypeFont::create(fontData);
    }
} // namespace p5
