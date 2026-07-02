#include <p5cpp/application/window.hpp>
#include <p5cpp/application/logging.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <string>
#include <stdexcept>

namespace p5cpp
{
    class GLFWWindow : public Window
    {
    public:
        static std::unique_ptr<GLFWWindow> create(int width, int height, std::string_view title)
        {
            try {
                return std::unique_ptr<GLFWWindow>(new GLFWWindow(width, height, title));
            } catch (const std::exception& e) {
                return nullptr;
            }
        }

        ~GLFWWindow() override
        {
            if (window) {
                glfwDestroyWindow(window);
            }

            glfwTerminate();
        }

        void setSize(int width, int height) override
        {
            glfwSetWindowSize(window, width, height);
        }

        void setPosition(int x, int y) override
        {
            glfwSetWindowPos(window, x, y);
        }

        void setTitle(std::string_view title) override
        {
            glfwSetWindowTitle(window, title.data());
        }

        void setResizable(bool resizable) override
        {
            glfwSetWindowAttrib(window, GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
        }

        void setEventCallback(std::function<void(const WindowEvent&)> callback) override
        {
            eventCallback = std::move(callback);
        }

        void setVisible(bool visible) override
        {
            if (visible) {
                glfwShowWindow(window);
            } else {
                glfwHideWindow(window);
            }
        }

        void swapBuffers() override
        {
            glfwSwapBuffers(window);
        }

        void pollEvents() override
        {
            glfwPollEvents();
        }

        int2 getMousePosition() override
        {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            return int2 {static_cast<int>(xpos), static_cast<int>(ypos)};
        }

        int2 getLogicalSize() override
        {
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            return int2 {width, height};
        }

        int2 getPhysicalSize() override
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            return int2 {width, height};
        }

    private:
        GLFWWindow(int width, int height, std::string_view title)
            : window(nullptr)
        {
            glfwSetErrorCallback(&errorCallback);

            // Initialize GLFW and create a window
            if (not glfwInit()) {
                throw std::runtime_error("Failed to initialize GLFW");
            }

            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
            // glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

            window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
            if (window == nullptr) {
                glfwTerminate();
                throw std::runtime_error("Failed to create GLFW window");
            }

            glfwMakeContextCurrent(window);
            if (not gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&glfwGetProcAddress))) {
                glfwDestroyWindow(window);
                glfwTerminate();
                throw std::runtime_error("Failed to initialize GLAD");
            }

            glfwSwapInterval(1);
            glfwSetWindowUserPointer(window, this);

            glfwSetWindowCloseCallback(window, &closeCallback);
            glfwSetFramebufferSizeCallback(window, &framebufferSizeCallback);
            glfwSetWindowSizeCallback(window, &windowSizeCallback);
            glfwSetCursorPosCallback(window, &mouseMoveCallback);
            glfwSetMouseButtonCallback(window, &mouseButtonCallback);
            glfwSetScrollCallback(window, &mouseScrollCallback);
            glfwSetKeyCallback(window, &keyCallback);
            glfwSetCharCallback(window, &characterCallback);
        }

        void publish(const WindowEvent& event)
        {
            if (eventCallback != nullptr) {
                eventCallback(event);
            }
        }

        static void closeCallback(GLFWwindow* window)
        {
            auto self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            if (self != nullptr) {
                WindowEvent event;
                event.type = EventType::close;
                self->publish(event);
            }
        }

        static void framebufferSizeCallback(GLFWwindow* window, int width, int height)
        {
            auto self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            if (self != nullptr) {
                WindowEvent event;
                event.type = EventType::framebufferResize;
                event.framebufferResize.width = width;
                event.framebufferResize.height = height;
                self->publish(event);
            }
        }

        static void windowSizeCallback(GLFWwindow* window, int width, int height)
        {
            auto self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            if (self != nullptr) {
                WindowEvent event;
                event.type = EventType::windowResize;
                event.windowResize.width = width;
                event.windowResize.height = height;
                self->publish(event);
            }
        }

        static void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
        {
            auto self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            if (self != nullptr) {
                WindowEvent event;
                event.type = EventType::mouseMove;
                event.mouseMove.x = xpos;
                event.mouseMove.y = ypos;
                self->publish(event);
            }
        }

        static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
        {
            auto self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            if (self != nullptr) {
                double mouseX, mouseY;
                glfwGetCursorPos(window, &mouseX, &mouseY);

                WindowEvent event;
                event.type = (action == GLFW_PRESS) ? EventType::mousePress : EventType::mouseRelease;
                event.mouseButton.button = convertMouseButton(button);
                event.mouseButton.x = static_cast<double>(mouseX);
                event.mouseButton.y = static_cast<double>(mouseY);
                self->publish(event);
            }
        }

        static void mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
        {
            auto self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            if (self != nullptr) {
                double mouseX, mouseY;
                glfwGetCursorPos(window, &mouseX, &mouseY);

                WindowEvent event;
                event.type = EventType::mouseScroll;
                event.mouseScroll.dx = xoffset;
                event.mouseScroll.dy = yoffset;
                event.mouseScroll.x = static_cast<double>(mouseX);
                event.mouseScroll.y = static_cast<double>(mouseY);
                self->publish(event);
            }
        }

        static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            auto self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            if (self != nullptr) {
                EventType eventType = std::invoke([action]() {
                    switch (action) {
                        case GLFW_PRESS: return EventType::keyPress;
                        case GLFW_RELEASE: return EventType::keyRelease;
                        case GLFW_REPEAT: return EventType::keyRepeat;
                        default: throw std::runtime_error("Unknown key action");
                    }
                });

                WindowEvent event;
                event.type = eventType;
                event.keyEvent.key = convertKey(key);
                event.keyEvent.mods = mods;
                self->publish(event);
            }
        }

        static void characterCallback(GLFWwindow* window, unsigned int codepoint)
        {
            auto self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            if (self != nullptr) {
                WindowEvent event;
                event.type = EventType::character;
                event.charEvent.codepoint = static_cast<char32_t>(codepoint);
                self->publish(event);
            }
        }

        static void errorCallback(int errorCode, const char* description)
        {
            const std::string message = "GLFW Error (" + std::to_string(errorCode) + "): " + description;
            error(message);
        }

        static MouseButton convertMouseButton(int button)
        {
            switch (button) {
                case GLFW_MOUSE_BUTTON_LEFT:
                    return MouseButton::left;
                case GLFW_MOUSE_BUTTON_RIGHT:
                    return MouseButton::right;
                case GLFW_MOUSE_BUTTON_MIDDLE:
                    return MouseButton::middle;
                default:
                    return MouseButton::left; // Default to left button for unknown buttons
            }
        }

        static Key convertKey(int k)
        {
            switch (k) {
                // Printable
                case GLFW_KEY_SPACE: return Key::space;
                case GLFW_KEY_APOSTROPHE: return Key::apostrophe;
                case GLFW_KEY_COMMA: return Key::comma;
                case GLFW_KEY_MINUS: return Key::minus;
                case GLFW_KEY_PERIOD: return Key::period;
                case GLFW_KEY_SLASH: return Key::slash;
                case GLFW_KEY_0: return Key::n0;
                case GLFW_KEY_1: return Key::n1;
                case GLFW_KEY_2: return Key::n2;
                case GLFW_KEY_3: return Key::n3;
                case GLFW_KEY_4: return Key::n4;
                case GLFW_KEY_5: return Key::n5;
                case GLFW_KEY_6: return Key::n6;
                case GLFW_KEY_7: return Key::n7;
                case GLFW_KEY_8: return Key::n8;
                case GLFW_KEY_9: return Key::n9;
                case GLFW_KEY_SEMICOLON: return Key::semicolon;
                case GLFW_KEY_EQUAL: return Key::equal;
                case GLFW_KEY_LEFT_BRACKET: return Key::leftBracket;
                case GLFW_KEY_BACKSLASH: return Key::backslash;
                case GLFW_KEY_RIGHT_BRACKET: return Key::rightBracket;
                case GLFW_KEY_GRAVE_ACCENT: return Key::graveAccent;
                // Letters
                case GLFW_KEY_A: return Key::a;
                case GLFW_KEY_B: return Key::b;
                case GLFW_KEY_C: return Key::c;
                case GLFW_KEY_D: return Key::d;
                case GLFW_KEY_E: return Key::e;
                case GLFW_KEY_F: return Key::f;
                case GLFW_KEY_G: return Key::g;
                case GLFW_KEY_H: return Key::h;
                case GLFW_KEY_I: return Key::i;
                case GLFW_KEY_J: return Key::j;
                case GLFW_KEY_K: return Key::k;
                case GLFW_KEY_L: return Key::l;
                case GLFW_KEY_M: return Key::m;
                case GLFW_KEY_N: return Key::n;
                case GLFW_KEY_O: return Key::o;
                case GLFW_KEY_P: return Key::p;
                case GLFW_KEY_Q: return Key::q;
                case GLFW_KEY_R: return Key::r;
                case GLFW_KEY_S: return Key::s;
                case GLFW_KEY_T: return Key::t;
                case GLFW_KEY_U: return Key::u;
                case GLFW_KEY_V: return Key::v;
                case GLFW_KEY_W: return Key::w;
                case GLFW_KEY_X: return Key::x;
                case GLFW_KEY_Y: return Key::y;
                case GLFW_KEY_Z: return Key::z;
                // Control & navigation
                case GLFW_KEY_ESCAPE: return Key::escape;
                case GLFW_KEY_ENTER: return Key::enter;
                case GLFW_KEY_TAB: return Key::tab;
                case GLFW_KEY_BACKSPACE: return Key::backspace;
                case GLFW_KEY_INSERT: return Key::insert;
                case GLFW_KEY_DELETE: return Key::del;
                case GLFW_KEY_RIGHT: return Key::right;
                case GLFW_KEY_LEFT: return Key::left;
                case GLFW_KEY_DOWN: return Key::down;
                case GLFW_KEY_UP: return Key::up;
                case GLFW_KEY_PAGE_UP: return Key::pageUp;
                case GLFW_KEY_PAGE_DOWN: return Key::pageDown;
                case GLFW_KEY_HOME: return Key::home;
                case GLFW_KEY_END: return Key::end;
                // Function keys
                case GLFW_KEY_F1: return Key::f1;
                case GLFW_KEY_F2: return Key::f2;
                case GLFW_KEY_F3: return Key::f3;
                case GLFW_KEY_F4: return Key::f4;
                case GLFW_KEY_F5: return Key::f5;
                case GLFW_KEY_F6: return Key::f6;
                case GLFW_KEY_F7: return Key::f7;
                case GLFW_KEY_F8: return Key::f8;
                case GLFW_KEY_F9: return Key::f9;
                case GLFW_KEY_F10: return Key::f10;
                case GLFW_KEY_F11: return Key::f11;
                case GLFW_KEY_F12: return Key::f12;
                // Modifiers
                case GLFW_KEY_LEFT_SHIFT: return Key::leftShift;
                case GLFW_KEY_LEFT_CONTROL: return Key::leftCtrl;
                case GLFW_KEY_LEFT_ALT: return Key::leftAlt;
                case GLFW_KEY_LEFT_SUPER: return Key::leftSuper;
                case GLFW_KEY_RIGHT_SHIFT: return Key::rightShift;
                case GLFW_KEY_RIGHT_CONTROL: return Key::rightCtrl;
                case GLFW_KEY_RIGHT_ALT: return Key::rightAlt;
                case GLFW_KEY_RIGHT_SUPER: return Key::rightSuper;
                default: return Key::unknown;
            }
        }

        std::function<void(const WindowEvent&)> eventCallback;
        GLFWwindow* window;
    };
} // namespace p5cpp

namespace p5cpp
{
    std::unique_ptr<Window> Window::create(int width, int height, std::string_view title)
    {
        return GLFWWindow::create(width, height, title);
    }
} // namespace p5cpp
