#include "font.hpp"
#include <freetype/freetype.h>
#include <unordered_map>
#include <vector>
#include <glad/glad.h>

namespace p5
{
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

        explicit FreeTypeFont(const std::filesystem::path& fontFilePath, int size)
        {
            library = createLibrary();
            if (library == nullptr) {
                throw std::runtime_error("Failed to initialize FreeType library");
            }

            face = createFace(library.get(), fontFilePath);
            if (face == nullptr) {
                throw std::runtime_error("Failed to load font: " + fontFilePath.string());
            }

            FT_Set_Pixel_Sizes(face.get(), 0, size);
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

        const Glyph* getGlyph(char32_t codepoint) override
        {
            const auto itr = glyphs.find(codepoint);
            if (itr != glyphs.end()) {
                return &itr->second;
            }

            // At this point, we need to load the glyph for the given codepoint, we need to load its metadata and bitmap from FreeType,
            // add it to the current glyph page (or create a new page if the current one is full) and upload the bitmap to the GPU, then we can return the Glyph metadata.
            const FT_UInt glyphIndex = FT_Get_Char_Index(face.get(), codepoint);
            if (glyphIndex == 0) {
                return nullptr;
            }

            const FT_Error loadError = FT_Load_Glyph(face.get(), glyphIndex, FT_LOAD_DEFAULT);
            if (loadError) {
                return nullptr;
            }

            const FT_Error renderError = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
            if (renderError) {
                return nullptr;
            }

            const FT_GlyphSlot slot = face->glyph;
            const FT_Bitmap& bitmap = slot->bitmap;

            const uint2 glyphSize = {static_cast<uint32_t>(bitmap.width), static_cast<uint32_t>(bitmap.rows)};
            const uint2 bearing = {static_cast<uint32_t>(slot->bitmap_left), static_cast<uint32_t>(slot->bitmap_top)};
            const float2 advance = {static_cast<float>(slot->advance.x) / 64.0f, static_cast<float>(slot->advance.y) / 64.0f}; // FreeType uses 1/64th of a pixel as its unit for advance

            GlyphPage& glyphPage = getOrCreateGlyphPageForGlyph(glyphSize);

            const rect2f uvRect = putGlyphBitmapInPage(bitmap.buffer, glyphSize, glyphPage);
            const Glyph glyph = Glyph {
                .size = {static_cast<float>(glyphSize.x), static_cast<float>(glyphSize.y)},
                .bearing = {static_cast<float>(bearing.x), static_cast<float>(bearing.y)},
                .uvRect = uvRect,
                .advance = advance,
                .pageIndex = static_cast<GlyphPageIndex>(glyphPages.size() - 1),
            };

            return &glyphs.insert({codepoint, glyph}).first->second;
        }

        int getTextSize() const override
        {
            return face->size->metrics.y_ppem;
        }

        float getLineHeight() const override
        {
            return static_cast<float>(face->size->metrics.height) / 64.0f; // FreeType uses 1/64th of a pixel as its unit for line height
        }

    private:
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
        };

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

        std::unique_ptr<FT_LibraryRec_, FT_Deleter> library;
        std::unique_ptr<FT_FaceRec_, FT_Deleter> face;
        std::vector<GlyphPage> glyphPages;
        std::unordered_map<char32_t, Glyph> glyphs;
    };
} // namespace p5

namespace p5
{
    std::unique_ptr<Font> loadFont(const std::filesystem::path& fontFilePath, int size)
    {
        return std::make_unique<FreeTypeFont>(fontFilePath, size);
    }
} // namespace p5
