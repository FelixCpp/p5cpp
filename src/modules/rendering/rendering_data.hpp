#pragma once

#include <p5cpp/p5cpp.hpp>

#include "../../services/uniform_cache.hpp"
#include "../../services/renderer.hpp"
#include "../../render_state_stack.hpp"

#include <memory>

namespace p5cpp
{
    struct RenderingData
    {
        std::shared_ptr<Shader> defaultShader;
        std::shared_ptr<Shader> textShader;
        std::shared_ptr<Font> defaultFont;
        std::shared_ptr<Framebuffer> defaultFramebuffer;
        std::shared_ptr<Texture> whiteTexture;
        std::shared_ptr<Renderer> renderer;
        std::shared_ptr<UniformCache> uniformCache;
        std::vector<Framebuffer*> framebufferStack;

        RenderStateStack renderStateStack;
    };
} // namespace p5cpp
