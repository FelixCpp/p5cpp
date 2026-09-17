#include <p5cpp/graphics/graphics_plugin.hpp>
#include <p5cpp/window/window.hpp>

namespace p5
{
    GraphicsPlugin::GraphicsPlugin()
        : m_gpuDevice(nullptr),
          m_blitPass(nullptr),
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

        m_blitPass = std::make_unique<BlitPass>(*m_gpuDevice);
        m_canvas = std::make_unique<Canvas>(*m_gpuDevice);
        provideDependency(m_canvas.get());
        provideDependency(this);

        m_size = window.getLogicalSize();

        recreateDefaultGraphics();

        withDefaultGraphics(next);
    }

    void GraphicsPlugin::event(const Next& next, const WindowEvent& event)
    {
        if (const auto* resize = event.as_if<WindowEvent::WindowResize>()) {
            handleWindowResize(resize->width, resize->height);
        }

        if (const auto* resize = event.as_if<WindowEvent::FramebufferResize>()) {
            handleFramebufferResize(resize->width, resize->height);
        }

        withDefaultGraphics(next);
    }

    void GraphicsPlugin::draw(const Next& next)
    {
        withDefaultGraphics(next);

        if (m_defaultGraphics.isValid()) {
            m_blitPass->blit(m_defaultGraphics);
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
        m_blitPass.reset();
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

    void GraphicsPlugin::withDefaultGraphics(const Next& next)
    {
        m_canvas->pushGraphics(m_defaultGraphics, true);
        next();
        m_canvas->popGraphics();
    }

    void GraphicsPlugin::handleWindowResize(uint32_t width, uint32_t height)
    {
        const bool isWindowMinimized = width == 0 or height == 0;
        if (isWindowMinimized) {
            return;
        }

        m_size = uint2 {.x = width, .y = height};
        recreateDefaultGraphics();
    }

    void GraphicsPlugin::handleFramebufferResize(uint32_t width, uint32_t height)
    {
        m_gpuDevice->reconfigure(width, height);
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
