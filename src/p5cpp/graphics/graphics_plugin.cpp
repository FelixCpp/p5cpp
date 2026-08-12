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

        // Push the default framebuffer around the rest of the setup chain too (not just draw(), see
        // below) so Sketch::setup() can call background()/draw calls just like Sketch::draw() can.
        m_graphics->pushFramebuffer(m_defaultFramebuffer);
        next();
        m_graphics->popFramebuffer();
    }

    void GraphicsPlugin::event(Context& context, const Next& next, const WindowEvent& event)
    {
        if (event.is<WindowEvent::WindowResize>()) {
            const auto& resize = event.as<WindowEvent::WindowResize>();
            // Ignore 0x0 (e.g. minimize on some platforms): framebuffer/renderbuffer storage of that
            // size is degenerate and would just be recreated again on the next real resize anyway.
            if (resize.width > 0 and resize.height > 0) {
                m_defaultFramebuffer = createFramebuffer(resize.width, resize.height);
            }
        }

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
