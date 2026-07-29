#pragma once

#include <p5cpp/graphics/texture.hpp>

namespace p5cpp::detail
{
    // Not part of the public API (this header lives under src/, not include/p5cpp/,
    // and is never pulled in by p5cpp.hpp) - the only legitimate way to mint a
    // single-channel (r8) glyph atlas Texture, used exclusively by Font's GlyphAtlas
    // (font.cpp). Exists as a friended class rather than a friended free function so
    // that Texture's public header only has to forward-declare an opaque name (see
    // texture.hpp) instead of exposing a callable declaration to every consumer of
    // Texture.
    class GlyphAtlasTextureFactory
    {
    public:
        static Texture make(uint32_t width, uint32_t height);
    };
} // namespace p5cpp::detail
