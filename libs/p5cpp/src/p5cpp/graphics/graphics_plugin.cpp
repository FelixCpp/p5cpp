#include <p5cpp/graphics/graphics_plugin.hpp>
#include <p5cpp/window/window.hpp>

namespace p5
{
    GraphicsPlugin::GraphicsPlugin()
        : m_gpuDevice(nullptr),
          m_canvas(nullptr),
          m_defaultGraphics(),
          m_size(0, 0),
          m_samples(4)
    {
    }

    void GraphicsPlugin::setup(const Next& next)
    {
        Window& window = requireDependency<Window>();

        m_gpuDevice = GpuDevice::create(window);
        if (m_gpuDevice == nullptr) {
            throw std::runtime_error("Failed to create GpuDevice (WebGPU initialization failed)");
        }

        provideDependency(m_gpuDevice.get());

        m_canvas = std::make_unique<Canvas>();
        provideDependency(m_canvas.get());
        provideDependency(this);

        m_size = window.getLogicalSize();

        recreateDefaultGraphics();

        m_canvas->pushGraphics(m_defaultGraphics, true);
        next();
        m_canvas->popGraphics();
    }

    void GraphicsPlugin::event(const Next& next, const WindowEvent& event)
    {
        if (const auto* resize = event.as_if<WindowEvent::WindowResize>()) {
            const auto isWindowMinimized = resize->width == 0 or resize->height == 0;
            if (not isWindowMinimized) {
                m_size = uint2 {.x = resize->width, .y = resize->height};
                recreateDefaultGraphics();
            }
        }

        if (const auto* resize = event.as_if<WindowEvent::FramebufferResize>()) {
            m_gpuDevice->reconfigure(resize->width, resize->height);
        }

        m_canvas->pushGraphics(m_defaultGraphics, true);
        next();
        m_canvas->popGraphics();
    }

    void GraphicsPlugin::draw(const Next& next)
    {
        m_canvas->pushGraphics(m_defaultGraphics, true);
        next();
        m_canvas->popGraphics();

        Window& window = requireDependency<Window>();
        const uint2& size = window.getPhysicalSize();
        if (m_defaultGraphics.isValid()) {
            blitGraphicsToScreen(m_defaultGraphics, size.x, size.y);
        }
    }

    void GraphicsPlugin::destroy(const Next& next)
    {
        next();

        removeDependency<Canvas>();
        removeDependency<GraphicsPlugin>();
        removeDependency<GpuDevice>();

        m_defaultGraphics = Graphics {};

        m_canvas.reset();
        m_gpuDevice.reset();
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
