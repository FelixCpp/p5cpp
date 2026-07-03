#pragma once

#include <p5cpp/graphics/shader.hpp>
#include <p5cpp/graphics/uniform_cache.hpp>
#include <p5cpp/graphics/renderer.hpp>
#include <p5cpp/graphics/render_state_stack.hpp>

#include "../../render_state_stack.hpp"

#include <memory>

namespace p5cpp
{
    struct RenderingData
    {
        std::shared_ptr<ShaderImpl> defaultShader;
        std::shared_ptr<ShaderImpl> textShader;
        std::shared_ptr<Font> defaultFont;
        std::shared_ptr<FramebufferImpl> defaultFramebuffer;
        std::shared_ptr<Texture> whiteTexture;
        std::shared_ptr<Renderer> renderer;
        std::shared_ptr<UniformCache> uniformCache;
        std::vector<FramebufferImpl*> framebufferStack;

        RenderStateStack renderStateStack;
    };
} // namespace p5cpp
