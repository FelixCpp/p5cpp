#include <p5cpp/window/window.hpp>
#include <iostream>

#include <glad/glad.h>

namespace p5
{
    static Key mapKey(int glfwKey)
    {
        switch (glfwKey) {
            case GLFW_KEY_SPACE: return Key::Space;
            case GLFW_KEY_APOSTROPHE: return Key::Apostrophe;
            case GLFW_KEY_COMMA: return Key::Comma;
            case GLFW_KEY_MINUS: return Key::Minus;
            case GLFW_KEY_PERIOD: return Key::Period;
            case GLFW_KEY_SLASH: return Key::Slash;
            case GLFW_KEY_0: return Key::Num0;
            case GLFW_KEY_1: return Key::Num1;
            case GLFW_KEY_2: return Key::Num2;
            case GLFW_KEY_3: return Key::Num3;
            case GLFW_KEY_4: return Key::Num4;
            case GLFW_KEY_5: return Key::Num5;
            case GLFW_KEY_6: return Key::Num6;
            case GLFW_KEY_7: return Key::Num7;
            case GLFW_KEY_8: return Key::Num8;
            case GLFW_KEY_9: return Key::Num9;
            case GLFW_KEY_SEMICOLON: return Key::Semicolon;
            case GLFW_KEY_EQUAL: return Key::Equal;
            case GLFW_KEY_A: return Key::A;
            case GLFW_KEY_B: return Key::B;
            case GLFW_KEY_C: return Key::C;
            case GLFW_KEY_D: return Key::D;
            case GLFW_KEY_E: return Key::E;
            case GLFW_KEY_F: return Key::F;
            case GLFW_KEY_G: return Key::G;
            case GLFW_KEY_H: return Key::H;
            case GLFW_KEY_I: return Key::I;
            case GLFW_KEY_J: return Key::J;
            case GLFW_KEY_K: return Key::K;
            case GLFW_KEY_L: return Key::L;
            case GLFW_KEY_M: return Key::M;
            case GLFW_KEY_N: return Key::N;
            case GLFW_KEY_O: return Key::O;
            case GLFW_KEY_P: return Key::P;
            case GLFW_KEY_Q: return Key::Q;
            case GLFW_KEY_R: return Key::R;
            case GLFW_KEY_S: return Key::S;
            case GLFW_KEY_T: return Key::T;
            case GLFW_KEY_U: return Key::U;
            case GLFW_KEY_V: return Key::V;
            case GLFW_KEY_W: return Key::W;
            case GLFW_KEY_X: return Key::X;
            case GLFW_KEY_Y: return Key::Y;
            case GLFW_KEY_Z: return Key::Z;
            case GLFW_KEY_LEFT_BRACKET: return Key::LeftBracket;
            case GLFW_KEY_BACKSLASH: return Key::Backslash;
            case GLFW_KEY_RIGHT_BRACKET: return Key::RightBracket;
            case GLFW_KEY_GRAVE_ACCENT: return Key::GraveAccent;
            case GLFW_KEY_WORLD_1: return Key::World1;
            case GLFW_KEY_WORLD_2: return Key::World2;
            case GLFW_KEY_ESCAPE: return Key::Escape;
            case GLFW_KEY_ENTER: return Key::Enter;
            case GLFW_KEY_TAB: return Key::Tab;
            case GLFW_KEY_BACKSPACE: return Key::Backspace;
            case GLFW_KEY_INSERT: return Key::Insert;
            case GLFW_KEY_DELETE: return Key::Delete;
            case GLFW_KEY_RIGHT: return Key::Right;
            case GLFW_KEY_LEFT: return Key::Left;
            case GLFW_KEY_DOWN: return Key::Down;
            case GLFW_KEY_UP: return Key::Up;
            case GLFW_KEY_PAGE_UP: return Key::PageUp;
            case GLFW_KEY_PAGE_DOWN: return Key::PageDown;
            case GLFW_KEY_HOME: return Key::Home;
            case GLFW_KEY_END: return Key::End;
            case GLFW_KEY_CAPS_LOCK: return Key::CapsLock;
            case GLFW_KEY_SCROLL_LOCK: return Key::ScrollLock;
            case GLFW_KEY_NUM_LOCK: return Key::NumLock;
            case GLFW_KEY_PRINT_SCREEN: return Key::PrintScreen;
            case GLFW_KEY_PAUSE: return Key::Pause;
            case GLFW_KEY_F1: return Key::F1;
            case GLFW_KEY_F2: return Key::F2;
            case GLFW_KEY_F3: return Key::F3;
            case GLFW_KEY_F4: return Key::F4;
            case GLFW_KEY_F5: return Key::F5;
            case GLFW_KEY_F6: return Key::F6;
            case GLFW_KEY_F7: return Key::F7;
            case GLFW_KEY_F8: return Key::F8;
            case GLFW_KEY_F9: return Key::F9;
            case GLFW_KEY_F10: return Key::F10;
            case GLFW_KEY_F11: return Key::F11;
            case GLFW_KEY_F12: return Key::F12;
            case GLFW_KEY_F13: return Key::F13;
            case GLFW_KEY_F14: return Key::F14;
            case GLFW_KEY_F15: return Key::F15;
            case GLFW_KEY_F16: return Key::F16;
            case GLFW_KEY_F17: return Key::F17;
            case GLFW_KEY_F18: return Key::F18;
            case GLFW_KEY_F19: return Key::F19;
            case GLFW_KEY_F20: return Key::F20;
            case GLFW_KEY_F21: return Key::F21;
            case GLFW_KEY_F22: return Key::F22;
            case GLFW_KEY_F23: return Key::F23;
            case GLFW_KEY_F24: return Key::F24;
            case GLFW_KEY_F25: return Key::F25;
            case GLFW_KEY_KP_0: return Key::Keypad0;
            case GLFW_KEY_KP_1: return Key::Keypad1;
            case GLFW_KEY_KP_2: return Key::Keypad2;
            case GLFW_KEY_KP_3: return Key::Keypad3;
            case GLFW_KEY_KP_4: return Key::Keypad4;
            case GLFW_KEY_KP_5: return Key::Keypad5;
            case GLFW_KEY_KP_6: return Key::Keypad6;
            case GLFW_KEY_KP_7: return Key::Keypad7;
            case GLFW_KEY_KP_8: return Key::Keypad8;
            case GLFW_KEY_KP_9: return Key::Keypad9;
            case GLFW_KEY_KP_DECIMAL: return Key::KeypadDecimal;
            case GLFW_KEY_KP_DIVIDE: return Key::KeypadDivide;
            case GLFW_KEY_KP_MULTIPLY: return Key::KeypadMultiply;
            case GLFW_KEY_KP_SUBTRACT: return Key::KeypadSubtract;
            case GLFW_KEY_KP_ADD: return Key::KeypadAdd;
            case GLFW_KEY_KP_ENTER: return Key::KeypadEnter;
            case GLFW_KEY_KP_EQUAL: return Key::KeypadEqual;
            case GLFW_KEY_LEFT_SHIFT: return Key::LeftShift;
            case GLFW_KEY_LEFT_CONTROL: return Key::LeftControl;
            case GLFW_KEY_LEFT_ALT: return Key::LeftAlt;
            case GLFW_KEY_LEFT_SUPER: return Key::LeftSuper;
            case GLFW_KEY_RIGHT_SHIFT: return Key::RightShift;
            case GLFW_KEY_RIGHT_CONTROL: return Key::RightControl;
            case GLFW_KEY_RIGHT_ALT: return Key::RightAlt;
            case GLFW_KEY_RIGHT_SUPER: return Key::RightSuper;
            case GLFW_KEY_MENU: return Key::Menu;
            default: return Key::Unknown;
        }
    }

    static MouseButton mapMouseButton(int glfwButton)
    {
        switch (glfwButton) {
            case GLFW_MOUSE_BUTTON_LEFT: return MouseButton::Left;
            case GLFW_MOUSE_BUTTON_RIGHT: return MouseButton::Right;
            case GLFW_MOUSE_BUTTON_MIDDLE: return MouseButton::Middle;
            case GLFW_MOUSE_BUTTON_4: return MouseButton::Button4;
            case GLFW_MOUSE_BUTTON_5: return MouseButton::Button5;
            case GLFW_MOUSE_BUTTON_6: return MouseButton::Button6;
            case GLFW_MOUSE_BUTTON_7: return MouseButton::Button7;
            case GLFW_MOUSE_BUTTON_8: return MouseButton::Button8;
            default: return MouseButton::Button8;
        }
    }

    static KeyMods mapMods(int glfwMods)
    {
        return KeyMods {
            .shift = (glfwMods & GLFW_MOD_SHIFT) != 0,
            .control = (glfwMods & GLFW_MOD_CONTROL) != 0,
            .alt = (glfwMods & GLFW_MOD_ALT) != 0,
            .super = (glfwMods & GLFW_MOD_SUPER) != 0,
            .capsLock = (glfwMods & GLFW_MOD_CAPS_LOCK) != 0,
            .numLock = (glfwMods & GLFW_MOD_NUM_LOCK) != 0,
        };
    }
} // namespace p5

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

        const std::string titleStr(title);
        GLFWwindow* window = glfwCreateWindow(width, height, titleStr.c_str(), nullptr, nullptr);
        if (window == nullptr) {
            glfwTerminate();
            return nullptr;
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable V-Sync

        if (not gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
            glfwDestroyWindow(window);
            glfwTerminate();
            return nullptr;
        }

        Window* instance = new Window {window, eventCallback};
        instance->m_title = titleStr;
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

        glfwSetWindowPosCallback(window, [](GLFWwindow* window, int x, int y) {
            Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));

            instance->publish(WindowEvent {
                WindowEvent::WindowMove {
                    .x = static_cast<int32_t>(x),
                    .y = static_cast<int32_t>(y),
                },
            });
        });

        glfwSetWindowIconifyCallback(window, [](GLFWwindow* window, int iconified) {
            Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));

            if (iconified) {
                instance->publish(WindowEvent {WindowEvent::WindowMinimize {}});
            } else {
                instance->publish(WindowEvent {WindowEvent::WindowRestore {}});
            }
        });

        glfwSetWindowMaximizeCallback(window, [](GLFWwindow* window, int maximized) {
            Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));

            if (maximized) {
                instance->publish(WindowEvent {WindowEvent::WindowMaximize {}});
            } else {
                instance->publish(WindowEvent {WindowEvent::WindowUnmaximize {}});
            }
        });

        glfwSetWindowContentScaleCallback(window, [](GLFWwindow* window, float xScale, float yScale) {
            Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));

            instance->publish(WindowEvent {
                WindowEvent::WindowContentScaleChange {
                    .xScale = xScale,
                    .yScale = yScale,
                },
            });
        });

        glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));

            if (action == GLFW_PRESS || action == GLFW_REPEAT) {
                instance->publish(WindowEvent {
                    WindowEvent::KeyPress {
                        .key = mapKey(key),
                        .scancode = scancode,
                        .mods = mapMods(mods),
                        .repeat = action == GLFW_REPEAT,
                    },
                });
            } else if (action == GLFW_RELEASE) {
                instance->publish(WindowEvent {
                    WindowEvent::KeyRelease {
                        .key = mapKey(key),
                        .scancode = scancode,
                        .mods = mapMods(mods),
                    },
                });
            }
        });

        glfwSetCharCallback(window, [](GLFWwindow* window, unsigned int codepoint) {
            Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));

            instance->publish(WindowEvent {
                WindowEvent::CharInput {
                    .codepoint = static_cast<uint32_t>(codepoint),
                },
            });
        });

        glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
            if (button < GLFW_MOUSE_BUTTON_1 || button > GLFW_MOUSE_BUTTON_8) {
                return;
            }

            Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));

            double x, y;
            glfwGetCursorPos(window, &x, &y);

            if (action == GLFW_PRESS) {
                instance->publish(WindowEvent {
                    WindowEvent::MouseButtonPress {
                        .button = mapMouseButton(button),
                        .mods = mapMods(mods),
                        .x = x,
                        .y = y,
                    },
                });
            } else if (action == GLFW_RELEASE) {
                instance->publish(WindowEvent {
                    WindowEvent::MouseButtonRelease {
                        .button = mapMouseButton(button),
                        .mods = mapMods(mods),
                        .x = x,
                        .y = y,
                    },
                });
            }
        });

        glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x, double y) {
            Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));

            instance->publish(WindowEvent {
                WindowEvent::MouseMove {
                    .x = x,
                    .y = y,
                },
            });
        });

        glfwSetScrollCallback(window, [](GLFWwindow* window, double xOffset, double yOffset) {
            Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));

            instance->publish(WindowEvent {
                WindowEvent::MouseScroll {
                    .xOffset = xOffset,
                    .yOffset = yOffset,
                },
            });
        });

        glfwSetCursorEnterCallback(window, [](GLFWwindow* window, int entered) {
            Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));

            if (entered) {
                instance->publish(WindowEvent {WindowEvent::MouseEnter {}});
            } else {
                instance->publish(WindowEvent {WindowEvent::MouseLeave {}});
            }
        });

        glfwSetDropCallback(window, [](GLFWwindow* window, int pathCount, const char** paths) {
            Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));

            std::vector<std::string> droppedPaths;
            droppedPaths.reserve(static_cast<size_t>(pathCount));
            for (int i = 0; i < pathCount; ++i) {
                droppedPaths.emplace_back(paths[i]);
            }

            instance->publish(WindowEvent {
                WindowEvent::FileDrop {
                    .paths = std::move(droppedPaths),
                },
            });
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
        m_title = title;
        glfwSetWindowTitle(m_window, m_title.c_str());
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

    void Window::maximize()
    {
        glfwMaximizeWindow(m_window);
    }

    void Window::minimize()
    {
        glfwIconifyWindow(m_window);
    }

    bool Window::isMaximized() const
    {
        return glfwGetWindowAttrib(m_window, GLFW_MAXIMIZED) == GLFW_TRUE;
    }

    bool Window::isMinimized() const
    {
        return glfwGetWindowAttrib(m_window, GLFW_ICONIFIED) == GLFW_TRUE;
    }

    void Window::enterFullscreen()
    {
        if (GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor()) {
            const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);

            if (videoMode) {
                m_preFullscreenWindowRect = rect2i {
                    .left = static_cast<int>(getPosition().x),
                    .top = static_cast<int>(getPosition().y),
                    .width = static_cast<int>(getLogicalSize().x),
                    .height = static_cast<int>(getLogicalSize().y),
                };

                glfwSetWindowMonitor(m_window, primaryMonitor, 0, 0, videoMode->width, videoMode->height, videoMode->refreshRate);
            }
        }
    }

    void Window::leaveFullscreen(std::optional<rect2i> restoreRect)
    {
        if (m_preFullscreenWindowRect) {
            const rect2i& rect = restoreRect.value_or(*m_preFullscreenWindowRect);
            glfwSetWindowMonitor(m_window, nullptr, rect.left, rect.top, rect.width, rect.height, 0);
            m_preFullscreenWindowRect.reset();
        }
    }

    void Window::toggleFullscreen()
    {
        if (isFullscreen()) {
            leaveFullscreen();
        } else {
            enterFullscreen();
        }
    }

    bool Window::isFullscreen() const
    {
        return glfwGetWindowMonitor(m_window) != nullptr;
    }

    void Window::pollEvents()
    {
        glfwPollEvents();
    }

    void Window::swapBuffers()
    {
        glfwSwapBuffers(m_window);
    }

    uint2 Window::getPhysicalSize() const
    {
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);
        return uint2 {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    }

    uint2 Window::getLogicalSize() const
    {
        int width, height;
        glfwGetWindowSize(m_window, &width, &height);
        return uint2 {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    }

    int2 Window::getPosition() const
    {
        int x, y;
        glfwGetWindowPos(m_window, &x, &y);
        return int2 {static_cast<int32_t>(x), static_cast<int32_t>(y)};
    }

    std::string_view Window::getTitle() const
    {
        return m_title;
    }

    bool Window::isResizable() const
    {
        return glfwGetWindowAttrib(m_window, GLFW_RESIZABLE) == GLFW_TRUE;
    }

    bool Window::isVisible() const
    {
        return glfwGetWindowAttrib(m_window, GLFW_VISIBLE) == GLFW_TRUE;
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
