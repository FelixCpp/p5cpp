#include <p5cpp/window/window.hpp>
#include <iostream>

namespace p5
{
    std::unique_ptr<Window> Window::create(uint32_t width, uint32_t height, std::string_view title, const EventCallback& eventCallback)
    {
        glfwSetErrorCallback([](int error, const char* description) {
            std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
        });

        if (not glfwInit()) {
            return nullptr;
        }

        if (GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor()) {
            if (const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor)) {
            glfwWindowHint(GLFW_RED_BITS, videoMode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, videoMode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, videoMode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, videoMode->refreshRate);

                width = std::min(width, static_cast<uint32_t>(videoMode->width));
                height = std::min(height, static_cast<uint32_t>(videoMode->height));
            }

            int monitorLeft, monitorTop, monitorWidth, monitorHeight;
            glfwGetMonitorWorkarea(primaryMonitor, &monitorLeft, &monitorTop, &monitorWidth, &monitorHeight);
            const int windowLeft = monitorLeft + (monitorWidth - static_cast<int>(width)) / 2;
            const int windowTop = monitorTop + (monitorHeight - static_cast<int>(height)) / 2;
            glfwWindowHint(GLFW_POSITION_X, windowLeft);
            glfwWindowHint(GLFW_POSITION_Y, windowTop);
        }

        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
        glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

        GLFWwindow* window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
        if (window == nullptr) {
            glfwTerminate();
            return nullptr;
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable V-Sync

        Window* instance = new Window {window, eventCallback};
        glfwSetWindowUserPointer(window, instance);
        glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height) {
            Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));

            instance->publish(WindowEvent {
                WindowEvent::WindowResize {
                    .width = static_cast<uint32_t>(width),
                    .height = static_cast<uint32_t>(height),
                },
            });
        });

        glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
            Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));

            instance->publish(WindowEvent {
                WindowEvent::FramebufferResize {
                    .width = static_cast<uint32_t>(width),
                    .height = static_cast<uint32_t>(height),
                },
            });
        });

        glfwSetWindowCloseCallback(window, [](GLFWwindow* window) {
            Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));

            instance->publish(WindowEvent {
                WindowEvent::Close {},
            });
        });

        glfwSetWindowFocusCallback(window, [](GLFWwindow* window, int focused) {
            Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));

            if (focused) {
                instance->publish(WindowEvent {
                    WindowEvent::FocusGained {},
                });
            } else {
                instance->publish(WindowEvent {
                    WindowEvent::FocusLost {},
                });
            }
        });

        return std::unique_ptr<Window>(instance);
    }

    Window::~Window()
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    void Window::setSize(uint32_t width, uint32_t height)
    {
        glfwSetWindowSize(m_window, static_cast<int>(width), static_cast<int>(height));
    }

    void Window::setPosition(int32_t x, int32_t y)
    {
        glfwSetWindowPos(m_window, static_cast<int>(x), static_cast<int>(y));
    }

    void Window::setTitle(std::string_view title)
    {
        glfwSetWindowTitle(m_window, title.data());
    }

    void Window::setResizable(bool resizable)
    {
        glfwSetWindowAttrib(m_window, GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
    }

    void Window::setVisible(bool visible)
    {
        if (visible) {
            glfwShowWindow(m_window);
        } else {
            glfwHideWindow(m_window);
        }
    }

    void Window::pollEvents()
    {
        glfwPollEvents();
    }

    void Window::swapBuffers()
    {
        glfwSwapBuffers(m_window);
    }

    Window::Window(GLFWwindow* window, const EventCallback& eventCallback)
        : m_window(window), m_eventCallback(eventCallback)
    {
    }
    void Window::publish(const WindowEvent& event)
    {
        if (m_eventCallback) {
            m_eventCallback(event);
        }
    }
} // namespace p5
