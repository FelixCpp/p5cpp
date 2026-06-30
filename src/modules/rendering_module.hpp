#pragma once

#include "../module.hpp"
#include "../renderer.hpp"
#include "../render_state_stack.hpp"
#include "p5cpp.hpp"

namespace p5cpp
{
    class RenderingModule : public Module
    {
    public:
        void setup(AppContext& context, Next next) override;
        void event(AppContext& context, WindowEvent& event, Next next) override;
        void draw(AppContext& context, Next next) override;
        void destroy(AppContext& context, Next next) override;

    private:
        Renderer renderer;
        RenderStateStack renderStateStack;

        std::shared_ptr<Shader> defaultShader;
        std::shared_ptr<Shader> textShader;
        std::shared_ptr<Font> defaultFont;
        std::shared_ptr<Framebuffer> defaultFramebuffer;
        std::shared_ptr<Texture> whiteTexture;

        bool needsDefaultCanvasRecreation;
    };
} // namespace p5cpp
