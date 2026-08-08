#include <p5cpp/window/window_plugin.hpp>
#include <p5cpp/application/kernel.hpp>

namespace p5
{
    WindowPlugin::WindowPlugin()
        : m_window(nullptr)
    {
    }

    void WindowPlugin::setup(Context& context, const Next& next)
    {
        m_window = Window::create(800, 600, "p5cpp", [this](const WindowEvent& event) {
            Kernel& kernel = getKernel();
            kernel.process(event);
        });

        context.provide(m_window.get());

        next();

        m_window->setVisible(true);
    }

    void WindowPlugin::event(Context& context, const Next& next, const WindowEvent& event)
    {
        next();
    }

    void WindowPlugin::draw(Context& context, const Next& next)
    {
        m_window->pollEvents();
        next();
        m_window->swapBuffers();
    }

    void WindowPlugin::destroy(Context& context, const Next& next)
    {
        next();

        m_window.reset();
    }
} // namespace p5
