#include "window.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <iostream>
#include <string>

namespace p5
{
    // -----------------------------------------------------------------------
    // GLFW key-code → Key enum mapping
    // -----------------------------------------------------------------------

    static Key glfw_key_to_key(int k)
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

    // -----------------------------------------------------------------------
    // AppWindow — owns GLFW handle, logical/physical dimensions, event callback
    // -----------------------------------------------------------------------

    struct AppWindow
    {
        GLFWwindow* handle = nullptr;
        int logicalWidth = 0;
        int logicalHeight = 0;
        int physicalWidth = 0;
        int physicalHeight = 0;
        WindowEventCallback onEvent;
    };

    // -----------------------------------------------------------------------
    // DefaultWindowFramebuffer
    //   Wraps FBO 0 (the real window framebuffer) so the renderer can bind it
    //   like any other Framebuffer without a blit step.
    //   getSize()         → logical points  (projection matrix, user coordinates)
    //   getViewportSize() → physical pixels (glViewport, correct on Retina/HiDPI)
    // -----------------------------------------------------------------------

    class DefaultWindowFramebuffer : public Framebuffer
    {
    public:
        explicit DefaultWindowFramebuffer(AppWindow* win) : m_win(win) {}

        uint32_t getTextureId() const override
        {
            return 0;
        }

        uint32_t getRendererId() const override
        {
            return 0;
        }

        uint2 getSize() const override
        {
            return {static_cast<uint32_t>(m_win->logicalWidth), static_cast<uint32_t>(m_win->logicalHeight)};
        }

        uint2 getViewportSize() const override
        {
            return {static_cast<uint32_t>(m_win->physicalWidth), static_cast<uint32_t>(m_win->physicalHeight)};
        }

        Texture* getColorTexture() override
        {
            return nullptr;
        }

    private:
        AppWindow* m_win;
    };

    // -----------------------------------------------------------------------
    // window_* implementations
    // -----------------------------------------------------------------------

    AppWindow* window_create(int width, int height, const char* title, WindowEventCallback onEvent)
    {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        auto* win = new AppWindow();
        win->logicalWidth = width;
        win->logicalHeight = height;
        win->onEvent = std::move(onEvent);

        win->handle = glfwCreateWindow(width, height, title, nullptr, nullptr);
        glfwMakeContextCurrent(win->handle);
        glfwSwapInterval(1);
        gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&glfwGetProcAddress));

        // On Retina the physical size is already larger than the logical size.
        glfwGetFramebufferSize(win->handle, &win->physicalWidth, &win->physicalHeight);

        std::cout << "Version: " << glGetString(GL_VERSION) << "\n";
        std::cout << "Vendor: " << glGetString(GL_VENDOR) << "\n";
        std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";
        std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";

        // Store win pointer so stateless GLFW callbacks can recover it.
        glfwSetWindowUserPointer(win->handle, win);

        glfwSetWindowCloseCallback(win->handle, [](GLFWwindow* h) {
            auto* win = static_cast<AppWindow*>(glfwGetWindowUserPointer(h));
            WindowEvent e;
            e.type = EventType::close;
            win->onEvent(e);
        });

        // Logical size changed → notify sketch so it can adapt layouts.
        glfwSetWindowSizeCallback(win->handle, [](GLFWwindow* h, int w, int ht) {
            auto* win = static_cast<AppWindow*>(glfwGetWindowUserPointer(h));
            win->logicalWidth = w;
            win->logicalHeight = ht;
            WindowEvent e;
            e.type = EventType::windowResize;
            e.windowResize = {w, ht};
            win->onEvent(e);
        });

        // Physical pixel size changed → fire framebufferResize event for default canvas recreation.
        glfwSetFramebufferSizeCallback(win->handle, [](GLFWwindow* h, int w, int ht) {
            auto* win = static_cast<AppWindow*>(glfwGetWindowUserPointer(h));
            win->physicalWidth = w;
            win->physicalHeight = ht;

            WindowEvent e;
            e.type = EventType::framebufferResize;
            e.framebufferResize = {w, ht};
            win->onEvent(e);
        });

        glfwSetMouseButtonCallback(win->handle, [](GLFWwindow* h, int button, int action, int) {
            auto* win = static_cast<AppWindow*>(glfwGetWindowUserPointer(h));
            double mx, my;
            glfwGetCursorPos(h, &mx, &my);
            const MouseButton btn =
                (button == GLFW_MOUSE_BUTTON_MIDDLE) ? MouseButton::middle : (button == GLFW_MOUSE_BUTTON_RIGHT) ? MouseButton::right
                                                                                                                 : MouseButton::left;
            WindowEvent e;
            e.mouseButton = {btn, static_cast<int>(mx), static_cast<int>(my)};
            if (action == GLFW_PRESS) {
                e.type = EventType::mousePress;
                win->onEvent(e);
            } else if (action == GLFW_RELEASE) {
                e.type = EventType::mouseRelease;
                win->onEvent(e);
            }
        });

        glfwSetCursorPosCallback(win->handle, [](GLFWwindow* h, double x, double y) {
            auto* win = static_cast<AppWindow*>(glfwGetWindowUserPointer(h));
            WindowEvent e;
            e.type = EventType::mouseMove;
            e.mouseMove = {static_cast<int>(x), static_cast<int>(y)};
            win->onEvent(e);
        });

        glfwSetScrollCallback(win->handle, [](GLFWwindow* h, double dx, double dy) {
            auto* win = static_cast<AppWindow*>(glfwGetWindowUserPointer(h));
            double mx, my;
            glfwGetCursorPos(h, &mx, &my);
            WindowEvent e;
            e.type = EventType::mouseScroll;
            e.mouseScroll = {static_cast<float>(dx), static_cast<float>(dy), static_cast<int>(mx), static_cast<int>(my)};
            win->onEvent(e);
        });

        glfwSetKeyCallback(win->handle, [](GLFWwindow* h, int key, int, int action, int mods) {
            auto* win = static_cast<AppWindow*>(glfwGetWindowUserPointer(h));
            WindowEvent e;
            e.keyEvent = {glfw_key_to_key(key), mods};
            if (action == GLFW_PRESS) e.type = EventType::keyPress;
            else if (action == GLFW_RELEASE) e.type = EventType::keyRelease;
            else if (action == GLFW_REPEAT) e.type = EventType::keyRepeat;
            else return;
            win->onEvent(e);
        });

        glfwSetCharCallback(win->handle, [](GLFWwindow* h, unsigned int codepoint) {
            auto* win = static_cast<AppWindow*>(glfwGetWindowUserPointer(h));
            WindowEvent e;
            e.type = EventType::character;
            e.charEvent = {static_cast<char32_t>(codepoint)};
            win->onEvent(e);
        });

        return win;
    }

    void window_destroy(AppWindow* win)
    {
        glfwDestroyWindow(win->handle);
        glfwTerminate();
        delete win;
    }

    void window_show(AppWindow* win) { glfwShowWindow(win->handle); }
    bool window_should_close(const AppWindow* win) { return glfwWindowShouldClose(win->handle); }
    void window_poll_events(AppWindow*) { glfwPollEvents(); }
    void window_swap_buffers(AppWindow* win) { glfwSwapBuffers(win->handle); }

    std::shared_ptr<Framebuffer> window_default_framebuffer(AppWindow* win)
    {
        return std::make_shared<DefaultWindowFramebuffer>(win);
    }

    void window_set_size(AppWindow* win, int width, int height)
    {
        glfwSetWindowSize(win->handle, width, height);
        win->logicalWidth = width;
        win->logicalHeight = height;

        glfwGetFramebufferSize(win->handle, &win->physicalWidth, &win->physicalHeight);
    }

    void window_set_title(AppWindow* win, std::string_view title) { glfwSetWindowTitle(win->handle, std::string(title).c_str()); }
    void window_set_resizable(AppWindow* win, bool resizable) { glfwSetWindowAttrib(win->handle, GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE); }

    int window_logical_width(const AppWindow* win) { return win->logicalWidth; }
    int window_logical_height(const AppWindow* win) { return win->logicalHeight; }
    int window_physical_width(const AppWindow* win) { return win->physicalWidth; }
    int window_physical_height(const AppWindow* win) { return win->physicalHeight; }

} // namespace p5
