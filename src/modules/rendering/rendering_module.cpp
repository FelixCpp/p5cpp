#include "rendering_module.hpp"

#include "../../shader.hpp"
#include "../../framebuffer.hpp"

#include "../../dejavusans.hpp"

#include "../input/input_data.hpp"

#include <p5cpp/application/app_context.hpp>

namespace p5cpp
{
    void RenderingModule::setup(AppContext& context, Next next)
    {
        info("RenderingModule setup");

        static constexpr size_t MAX_VERTICES = 65536;
        static constexpr size_t MAX_INDICES = 65536 * 3;

        data.renderer = Renderer::create(MAX_VERTICES, MAX_INDICES);
        data.renderStateStack = render_state_stack_create();

        const uint8_t whitePixel[4] = {255, 255, 255, 255};

        InputData& inputData = context.require<InputData>();

        data.defaultShader = create_default_shader();
        data.textShader = create_text_shader();
        data.defaultFont = loadFont({DejaVuSans_ttf, DejaVuSans_ttf_len});
        data.defaultFramebuffer = create_window_framebuffer(
            inputData.physicalWidth,
            inputData.physicalHeight,
            inputData.logicalWidth,
            inputData.logicalHeight
        );
        data.whiteTexture = createTexture(1, 1, whitePixel);
        data.uniformCache = std::make_unique<UniformCache>();
        needsDefaultCanvasRecreation = false;

        context.registerService<RenderingData>(&data);

        pushCanvas(data.defaultFramebuffer);
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
            InputData& inputData = context.require<InputData>();
            data.defaultFramebuffer = create_window_framebuffer(
                inputData.physicalWidth,
                inputData.physicalHeight,
                inputData.logicalWidth,
                inputData.logicalHeight
            );

            needsDefaultCanvasRecreation = false;
        }

        pushCanvas(data.defaultFramebuffer);
        next();
        popCanvas();

        blit_framebuffer_to_screen(*data.defaultFramebuffer);
    }

    void RenderingModule::destroy(AppContext& context, Next next)
    {
        next();

        context.unregisterService<RenderingData>();
        data = {};
    }
} // namespace p5cpp
