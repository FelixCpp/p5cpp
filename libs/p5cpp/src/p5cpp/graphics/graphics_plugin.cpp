#include <p5cpp/graphics/graphics_plugin.hpp>
#include <p5cpp/window/window.hpp>

namespace p5
{
    GraphicsPlugin::GraphicsPlugin()
        : m_canvas(nullptr),
          m_defaultGraphics(),
          m_size(0, 0),
          m_samples(4)
    {
    }

    void GraphicsPlugin::setup(Context& context, const Next& next)
    {
        m_canvas = std::make_unique<Canvas>();
        context.provide(m_canvas.get());
        context.provide(this);

        Window& window = context.require<Window>();
        m_size = window.getLogicalSize();

        recreateDefaultGraphics();

        m_canvas->pushGraphics(m_defaultGraphics, true);
        next();
        m_canvas->popGraphics();
    }

    void GraphicsPlugin::event(Context& context, const Next& next, const WindowEvent& event)
    {
        if (event.is<WindowEvent::WindowResize>()) {
            const auto& resize = event.as<WindowEvent::WindowResize>();
            const auto isWindowMinimized = resize.width == 0 or resize.height == 0;
            if (not isWindowMinimized) {
                m_size = uint2 {.x = resize.width, .y = resize.height};
                recreateDefaultGraphics();
            }
        }

        next();
    }

    void GraphicsPlugin::draw(Context& context, const Next& next)
    {
        m_canvas->pushGraphics(m_defaultGraphics, true);
        next();
        m_canvas->popGraphics();

        Window& window = context.require<Window>();
        const uint2& size = window.getPhysicalSize();
        if (m_defaultGraphics.isValid()) {
            blitGraphicsToScreen(m_defaultGraphics, size.x, size.y);
        }
    }

    void GraphicsPlugin::destroy(Context& context, const Next& next)
    {
        next();

        context.remove<GraphicsPlugin>();
        context.remove<Canvas>();
        m_canvas.reset();
    }

    void GraphicsPlugin::smooth(uint32_t samples)
    {
        m_samples = samples;
        recreateDefaultGraphics();
    }

    void GraphicsPlugin::noSmooth()
    {
        smooth(0);
    }

    void GraphicsPlugin::recreateDefaultGraphics()
    {
        if (auto recreated = createGraphics(m_size.x, m_size.y, m_samples)) {
            m_defaultGraphics = std::move(recreated).value();
        } else {
            error("GraphicsPlugin::recreateDefaultGraphics() failed; the default graphics target is unchanged");
        }
    }
} // namespace p5
