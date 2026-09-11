#include <p5cpp/window/window_plugin.hpp>
#include <p5cpp/application/kernel.hpp>

namespace p5
{
    WindowPlugin::WindowPlugin()
        : m_window(nullptr)
    {
    }

    void WindowPlugin::setup(const Next& next)
    {
        m_window = Window::create(800, 600, "p5cpp", [](const WindowEvent& event) {
            Kernel& kernel = getKernel();
            kernel.process(event);
        });

        if (m_window == nullptr) {
            throw std::runtime_error("Failed to create window (GLFW/OpenGL initialization failed)");
        }

        provideDependency(m_window.get());

        next();

        m_window->centerWindow();
        m_window->setVisible(true);
    }

    void WindowPlugin::event(const Next& next, [[maybe_unused]] const WindowEvent& event)
    {
        next();
    }

    void WindowPlugin::draw(const Next& next)
    {
        m_window->pollEvents();
        next();
        m_window->swapBuffers();
    }

    void WindowPlugin::destroy(const Next& next)
    {
        next();

        removeDependency<Window>();
        m_window.reset();
    }
} // namespace p5
