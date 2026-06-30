#include "rendering_module.hpp"
#include "../app_context.hpp"
#include "../shader.hpp"
#include "../framebuffer.hpp"

#include "../dejavusans.hpp"

namespace p5cpp
{
    void RenderingModule::setup(AppContext& context, Next next)
    {
        info("RenderingModule setup");

        static constexpr size_t MAX_VERTICES = 65536;
        static constexpr size_t MAX_INDICES = 65536 * 3;

        renderer = Renderer::create(MAX_VERTICES, MAX_INDICES);
        renderStateStack = render_state_stack_create();

        const uint8_t whitePixel[4] = {255, 255, 255, 255};

        defaultShader = create_default_shader();
        textShader = create_text_shader();
        defaultFont = loadFont({DejaVuSans_ttf, DejaVuSans_ttf_len});
        defaultFramebuffer = create_window_framebuffer(
            context.inputInfo.physicalWidth,
            context.inputInfo.physicalHeight,
            context.inputInfo.logicalWidth,
            context.inputInfo.logicalHeight
        );
        whiteTexture = createTexture(1, 1, whitePixel);
        needsDefaultCanvasRecreation = false;

        context.renderingInfo.defaultShader = defaultShader.get();
        context.renderingInfo.textShader = textShader.get();
        context.renderingInfo.defaultFont = defaultFont.get();
        context.renderingInfo.defaultFramebuffer = defaultFramebuffer.get();
        context.renderingInfo.whiteTexture = whiteTexture.get();
        context.renderingInfo.renderer = renderer.get();
        context.renderingInfo.renderStateStack = &renderStateStack;

        pushCanvas(defaultFramebuffer);
        next();
        popCanvas();
    }

    void RenderingModule::event(AppContext& context, WindowEvent& event, Next next)
    {
        if (event.type == EventType::framebufferResize) {
            needsDefaultCanvasRecreation = true;
        }

        next();
    }

    void RenderingModule::draw(AppContext& context, Next next)
    {
        if (needsDefaultCanvasRecreation) {
            defaultFramebuffer = create_window_framebuffer(
                context.inputInfo.physicalWidth,
                context.inputInfo.physicalHeight,
                context.inputInfo.logicalWidth,
                context.inputInfo.logicalHeight
            );

            needsDefaultCanvasRecreation = false;
        }

        pushCanvas(defaultFramebuffer);
        next();
        popCanvas();

        blit_framebuffer_to_screen(*defaultFramebuffer);
    }

    void RenderingModule::destroy(AppContext& context, Next next)
    {
        next();

        defaultShader.reset();
        textShader.reset();
        defaultFont.reset();
        defaultFramebuffer.reset();
        whiteTexture.reset();
    }
} // namespace p5cpp
