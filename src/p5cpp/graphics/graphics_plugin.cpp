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
        if (m_defaultFramebuffer == nullptr) {
            error("GraphicsPlugin::setup() failed to create the default framebuffer");
        }

        m_graphics->pushFramebuffer(m_defaultFramebuffer);
        next();
        m_graphics->popFramebuffer();
    }

    void GraphicsPlugin::event(Context& context, const Next& next, const WindowEvent& event)
    {
        if (event.is<WindowEvent::WindowResize>()) {
            const auto& resize = event.as<WindowEvent::WindowResize>();
            const auto isWindowMinimized = resize.width == 0 or resize.height == 0;
            if (not isWindowMinimized) {
                if (auto resized = createFramebuffer(resize.width, resize.height)) {
                    m_defaultFramebuffer = std::move(resized);
                } else {
                    error("GraphicsPlugin::event() failed to recreate the default framebuffer on resize; keeping the previous one");
                }
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
        if (m_defaultFramebuffer != nullptr) {
            blitFramebufferToScreen(m_defaultFramebuffer, size.x, size.y);
        }
    }

    void GraphicsPlugin::destroy(Context& context, const Next& next)
    {
        next();

        context.remove<Graphics>();
        m_graphics.reset();
    }
} // namespace p5
