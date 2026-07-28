#pragma once

#include <p5cpp/graphics/renderer.hpp>
#include <p5cpp/graphics/uniform_cache.hpp>
#include <p5cpp/graphics/framebuffer.hpp>
#include <p5cpp/graphics/shader.hpp>

namespace p5cpp
{
    // Draws a fullscreen quad sampling `source`'s color texture into `dest`, using
    // `shader`. Bypasses transform/tint/blend entirely - a screen-space pass is neither
    // transformed nor tinted, matching how background() bypasses them too. Leaves the
    // renderer flushed and ended against `dest` when done; begin()ing whatever should be
    // active afterward is the caller's job (mirrors NativeRenderer::begin/end pairing
    // elsewhere in this codebase).
    void drawFullscreenPass(NativeRenderer& renderer, UniformCache& uniformCache, const Framebuffer& source, const Framebuffer& dest, const Shader& shader);
} // namespace p5cpp
