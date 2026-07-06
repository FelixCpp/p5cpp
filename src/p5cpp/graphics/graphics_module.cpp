#include <p5cpp/graphics/graphics_module.hpp>
#include <p5cpp/application/app_context.hpp>

#include <p5cpp/application/input_component.hpp>

namespace p5cpp
{
    GraphicsModule::GraphicsModule()
        : m_component(nullptr)
    {
    }

    void GraphicsModule::setup(AppContext& context, Next next)
    {
        const InputComponent& inputData = context.require<InputComponent>();
        m_component = std::make_unique<GraphicsComponent>(inputData.getPhysicalWidth(), inputData.getPhysicalHeight());

        context.registerService(m_component.get());
        next();
    }

    void GraphicsModule::event(AppContext& context, WindowEvent& event, Next next)
    {
        if (event.type == EventType::framebufferResize) {
            m_component->resizeDefaultCanvas(event.framebufferResize.width, event.framebufferResize.height);
        }

        next();
    }

    void GraphicsModule::draw(AppContext& context, Next next)
    {
        m_component->beginFrame();
        next();
        m_component->endFrame();

        const InputComponent& inputData = context.require<InputComponent>();
        m_component->blitDefaultCanvasToScreen(inputData.getLogicalWidth(), inputData.getLogicalHeight());
    }

    void GraphicsModule::destroy(AppContext& context, Next next)
    {
        next();
        context.unregisterService<GraphicsComponent>();
    }
} // namespace p5cpp
