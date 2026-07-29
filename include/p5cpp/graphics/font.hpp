#pragma once

#include <p5cpp/graphics/texture.hpp>
#include <p5cpp/graphics/text.hpp>

#include <p5cpp/math/value2.hpp>
#include <p5cpp/math/rectangle.hpp>

#include <memory>
#include <span>
#include <vector>
#include <filesystem>

namespace p5cpp
{
    struct FontMetrics
    {
        float ascender;
        float descender;
        float lineHeight;
    };
} // namespace p5cpp

namespace p5cpp
{
    using TextContour = std::vector<float2>;
} // namespace p5cpp

namespace p5cpp
{
    struct ShapedGlyph
    {
        int2 bearing;
        int2 size;
        float_rect uvRect;
        size_t glyphAtlasIndex;
        float xOffset;     // HarfBuzz cluster-relative offset in pixels
        float yOffset;     // HarfBuzz cluster-relative offset in pixels
        float xAdvance;    // HarfBuzz advance in pixels
        float yAdvance;    // HarfBuzz advance in pixels
        bool isWhitespace; // true for space/tab — still advances but has no bitmap
        char32_t cluster;  // leading codepoint of the cluster (for word-wrap)
    };
} // namespace p5cpp

namespace p5cpp
{
    namespace detail
    {
        // Opaque - FreeType FT_Face, HarfBuzz hb_font_t, and the shaping/metrics/glyph
        // rasterization caches. No meaningful public representation, unlike
        // FontData::glyphAtlasPages below - defined only in font.cpp.
        struct FontRasterizer;
    } // namespace detail

    // Backing data for a Font, reachable via Font::data - kept separate from Font
    // itself (rather than being fields directly on Font) because glyphAtlasPages grows
    // over time as new glyphs get rasterized, and every Font copy must see that growth
    // immediately: they all alias the same FontData through the shared_ptr, so growth
    // is visible everywhere at once instead of only in whichever copy triggered it.
    struct FontData
    {
        // Declared out-of-line (defined in font.cpp): required because of the
        // unique_ptr<FontRasterizer> member below, an incomplete type at this point.
        ~FontData();

        // Current glyph atlas texture pages - grows as shape()/textToPoints() rasterize
        // glyphs that don't fit in an existing page. ShapedGlyph::glyphAtlasIndex is an
        // index into this. A page's Texture id/size never change once it exists (only
        // its pixel content does, via updateRegion()), so this is safe to read directly
        // without going through getGlyphAtlasTexture().
        std::vector<Texture> glyphAtlasPages;

        std::unique_ptr<detail::FontRasterizer> rasterizer;
    };

    // A default-constructed Font is invalid (data == nullptr) - the failure return of
    // loadFont() when the file can't be parsed, for example. Reference-counted like
    // Sound/AudioStream - copies alias the same FontData, freed automatically when the
    // last copy goes out of scope. No unload() - unlike Texture/Framebuffer/Shader,
    // this one keeps automatic cleanup.
    struct Font
    {
        std::shared_ptr<FontData> data;
    };

    Font loadFont(const std::filesystem::path& fontFilePath);
    Font loadFont(std::span<const uint8_t> fontData);

    bool isFontValid(const Font& font);

    std::vector<TextContour> textToPoints(const Font& font, std::string_view text, float x, float y, int textSize, int curveDetail = 8, float spacing = 0.0f, float maxWidth = -1.0f, TextWrap wrap = TextWrap::none);
    std::vector<ShapedGlyph> shape(const Font& font, std::string_view text, int textSize);
    const FontMetrics* getMetrics(const Font& font, int textSize);
    const Texture* getGlyphAtlasTexture(const Font& font, size_t glyphAtlasIndex);
} // namespace p5cpp
