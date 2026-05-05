#pragma once

#include <filesystem>
#include <memory>

#include "p5.hpp"

namespace p5
{
    typedef size_t GlyphPageIndex;

    struct Glyph
    {
        float2 size;
        float2 bearing;
        rect2f uvRect;
        float advance;
        GlyphPageIndex pageIndex;
    };

    struct Font
    {
        virtual ~Font() = default;
        virtual uint32_t getGlyphPageTextureId(GlyphPageIndex index) = 0;
        virtual const Glyph& getGlyph(char32_t codepoint) = 0;
    };

    std::unique_ptr<Font> loadFont(const std::filesystem::path& fontFilePath, int size);
} // namespace p5
