#include <p5cpp/graphics/graphics_module.hpp>
#include <p5cpp/application/app_context.hpp>

namespace p5cpp
{
    GraphicsModule::GraphicsModule()
    {
    }

    void GraphicsModule::setup(AppContext& context, Next next)
    {
        m_component = GraphicsComponent();

        context.registerService(&m_component);
        next();
    }

    void GraphicsModule::event(AppContext& context, WindowEvent& event, Next next)
    {
        next();
    }

    void GraphicsModule::draw(AppContext& context, Next next)
    {
        next();
    }

    void GraphicsModule::destroy(AppContext& context, Next next)
    {
        next();
        context.unregisterService<GraphicsComponent>();
    }
} // namespace p5cpp
