#include <p5cpp/graphics/graphics_plugin.hpp>
#include <p5cpp/graphics/framebuffer.hpp>
#include <p5cpp/window/window.hpp>

namespace p5
{
    void GraphicsPlugin::setup(Context& context, const Next& next)
    {
        m_graphics = std::make_unique<Graphics>();
        context.provide(m_graphics.get());

        Window& window = context.require<Window>();
        const uint2& size = window.getLogicalSize();

        m_defaultFramebuffer = createFramebuffer(size.x, size.y);

        next();
    }

    void GraphicsPlugin::event(Context& context, const Next& next, const WindowEvent& event)
    {
        next();
    }

    void GraphicsPlugin::draw(Context& context, const Next& next)
    {
        m_graphics->pushFramebuffer(m_defaultFramebuffer);
        next();
        m_graphics->popFramebuffer();

        Window& window = context.require<Window>();
        const uint2& size = window.getPhysicalSize();
        blitFramebufferToScreen(m_defaultFramebuffer, size.x, size.y);
    }

    void GraphicsPlugin::destroy(Context& context, const Next& next)
    {
        next();

        context.remove<Graphics>();
        m_graphics.reset();
    }
} // namespace p5
