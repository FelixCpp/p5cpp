#include <p5cpp/graphics/graphics_plugin.hpp>
#include <p5cpp/graphics/framebuffer.hpp>
#include <p5cpp/window/window.hpp>

namespace p5
{
    GraphicsPlugin::GraphicsPlugin()
        : m_graphics(nullptr),
          m_defaultFramebuffer(nullptr),
          m_size(0, 0),
          m_samples(4)
    {
    }

    void GraphicsPlugin::setup(Context& context, const Next& next)
    {
        m_graphics = std::make_unique<Graphics>();
        context.provide(m_graphics.get());
        context.provide(this);

        Window& window = context.require<Window>();
        m_size = window.getLogicalSize();

        recreateDefaultFramebuffer();

        m_graphics->pushFramebuffer(m_defaultFramebuffer, true);
        next();
        m_graphics->popFramebuffer();
    }

    void GraphicsPlugin::event(Context& context, const Next& next, const WindowEvent& event)
    {
        if (event.is<WindowEvent::WindowResize>()) {
            const auto& resize = event.as<WindowEvent::WindowResize>();
            const auto isWindowMinimized = resize.width == 0 or resize.height == 0;
            if (not isWindowMinimized) {
                m_size = uint2 {.x = resize.width, .y = resize.height};
                recreateDefaultFramebuffer();
            }
        }

        next();
    }

    void GraphicsPlugin::draw(Context& context, const Next& next)
    {
        m_graphics->pushFramebuffer(m_defaultFramebuffer, true);
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

        context.remove<GraphicsPlugin>();
        context.remove<Graphics>();
        m_graphics.reset();
    }

    void GraphicsPlugin::smooth(uint32_t samples)
    {
        m_samples = samples;
        recreateDefaultFramebuffer();
    }

    void GraphicsPlugin::noSmooth()
    {
        smooth(0);
    }

    void GraphicsPlugin::recreateDefaultFramebuffer()
    {
        if (auto recreated = createFramebuffer(m_size.x, m_size.y, m_samples)) {
            m_defaultFramebuffer = std::move(recreated);
        } else {
            error("GraphicsPlugin::recreateDefaultFramebuffer() failed; the default framebuffer is unchanged");
        }
    }
} // namespace p5
