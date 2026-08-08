#pragma once

#include <cstdint>
#include <variant>
#include <concepts>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <span>
#include <string>
#include <vector>

namespace p5
{
    // clang-format off
    enum class Key
    {
        Unknown, Space, Apostrophe, Comma, Minus, Period, Slash,
        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
        Semicolon, Equal,
        A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        LeftBracket, Backslash, RightBracket, GraveAccent, World1, World2, Escape, Enter, Tab, Backspace, Insert, Delete,
        Right, Left, Down, Up,
        PageUp, PageDown,
        Home, End, CapsLock, ScrollLock, NumLock, PrintScreen, Pause,
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25,
        Keypad0, Keypad1, Keypad2, Keypad3, Keypad4, Keypad5, Keypad6, Keypad7, Keypad8, Keypad9,
        KeypadDecimal, KeypadDivide, KeypadMultiply, KeypadSubtract, KeypadAdd, KeypadEnter, KeypadEqual,
        LeftShift, LeftControl, LeftAlt, LeftSuper, RightShift,
        RightControl, RightAlt, RightSuper, Menu,
        Count,
    };
    // clang-format on

    // clang-format off
    enum class MouseButton
    {
        Left, Right, Middle, Button4, Button5, Button6, Button7, Button8,
        Count,
    };
    // clang-format on

    struct KeyMods
    {
        bool shift;
        bool control;
        bool alt;
        bool super;
        bool capsLock;
        bool numLock;
    };

    class WindowEvent
    {
    public:
        struct EventTypeTag
        {
        };

        struct Close : EventTypeTag
        {
        };

        struct WindowResize : EventTypeTag
        {
            uint32_t width;
            uint32_t height;
        };

        struct FramebufferResize : EventTypeTag
        {
            uint32_t width;
            uint32_t height;
        };

        struct FocusGained : EventTypeTag
        {
        };

        struct FocusLost : EventTypeTag
        {
        };

        struct WindowMove : EventTypeTag
        {
            int32_t x;
            int32_t y;
        };

        struct WindowMinimize : EventTypeTag
        {
        };

        struct WindowRestore : EventTypeTag
        {
        };

        struct WindowMaximize : EventTypeTag
        {
        };

        struct WindowUnmaximize : EventTypeTag
        {
        };

        struct WindowContentScaleChange : EventTypeTag
        {
            float xScale;
            float yScale;
        };

        struct KeyPress : EventTypeTag
        {
            Key key;
            int scancode;
            KeyMods mods;
            bool repeat;
        };

        struct KeyRelease : EventTypeTag
        {
            Key key;
            int scancode;
            KeyMods mods;
        };

        struct CharInput : EventTypeTag
        {
            uint32_t codepoint;
        };

        struct MouseButtonPress : EventTypeTag
        {
            MouseButton button;
            KeyMods mods;
            double x;
            double y;
        };

        struct MouseButtonRelease : EventTypeTag
        {
            MouseButton button;
            KeyMods mods;
            double x;
            double y;
        };

        struct MouseMove : EventTypeTag
        {
            double x;
            double y;
        };

        struct MouseScroll : EventTypeTag
        {
            double xOffset;
            double yOffset;
        };

        struct MouseEnter : EventTypeTag
        {
        };

        struct MouseLeave : EventTypeTag
        {
        };

        struct FileDrop : EventTypeTag
        {
            std::vector<std::string> paths;
        };

        using EventType = std::variant<
            Close, WindowResize, FramebufferResize, FocusGained, FocusLost,
            WindowMove, WindowMinimize, WindowRestore, WindowMaximize, WindowUnmaximize, WindowContentScaleChange,
            KeyPress, KeyRelease, CharInput,
            MouseButtonPress, MouseButtonRelease, MouseMove, MouseScroll, MouseEnter, MouseLeave,
            FileDrop>;

        constexpr WindowEvent(const EventType& eventType);

        template <std::derived_from<EventTypeTag> T> constexpr bool is() const;
        template <std::derived_from<EventTypeTag> T> constexpr const T& as() const;

    private:
        EventType m_eventType;
    };
} // namespace p5

namespace p5
{
    struct Sketch
    {
        virtual ~Sketch() = default;
        virtual void setup() {}
        virtual void event(const WindowEvent& event) {}
        virtual void draw() {}
        virtual void destroy() {}
    };

    extern std::unique_ptr<Sketch> createSketch();
} // namespace p5

namespace p5
{
    bool isKeyDown(Key key);
    bool isKeyPressed(Key key);
    bool isKeyReleased(Key key);

    bool isMouseButtonDown(MouseButton button);
    bool isMouseButtonPressed(MouseButton button);
    bool isMouseButtonReleased(MouseButton button);

    double getMouseX();
    double getMouseY();
    double getPreviousMouseX();
    double getPreviousMouseY();
    double getMouseDeltaX();
    double getMouseDeltaY();

    double getScrollX();
    double getScrollY();

    bool isCursorInWindow();

    std::span<const std::string> getDroppedFiles();
} // namespace p5

namespace p5
{
    class Context
    {
    public:
        template <typename T> void provide(T* instance);
        template <typename T> void remove();
        template <typename T> T& require();
        template <typename T> T* get() const;
        template <typename T> bool has() const;

    private:
        std::unordered_map<std::type_index, void*> m_instances;
    };

} // namespace p5

namespace p5
{
    struct Plugin;

    class Next
    {
    public:
        explicit Next(std::span<const std::unique_ptr<Plugin>> chain, size_t index, Context* context, void (*step)(Plugin&, Context&, const Next&), const void* payload);
        void operator()() const;

        const void* getPayload() const;

    private:
        using Step = void (*)(Plugin&, Context&, const Next&);

        std::span<const std::unique_ptr<Plugin>> m_chain;
        size_t m_index;
        Context* context;
        Step m_step;
        const void* payload;
    };

    struct Plugin
    {
        virtual ~Plugin() = default;
        virtual void setup(Context& context, const Next& next);
        virtual void event(Context& context, const Next& next, const WindowEvent& event);
        virtual void draw(Context& context, const Next& next);
        virtual void destroy(Context& context, const Next& next);
    };
} // namespace p5

namespace p5
{
    constexpr WindowEvent::WindowEvent(const EventType& eventType)
        : m_eventType(std::move(eventType))
    {
    }

    template <std::derived_from<WindowEvent::EventTypeTag> T>
    constexpr bool WindowEvent::is() const
    {
        return std::holds_alternative<T>(m_eventType);
    }

    template <std::derived_from<WindowEvent::EventTypeTag> T>
    constexpr const T& WindowEvent::as() const
    {
        return std::get<T>(m_eventType);
    }
} // namespace p5

namespace p5
{
    template <typename T>
    void Context::provide(T* instance)
    {
        m_instances.try_emplace(typeid(T), instance);
    }

    template <typename T>
    void Context::remove()
    {
        m_instances.erase(typeid(T));
    }

    template <typename T>
    T& Context::require()
    {
        return *get<T>();
    }

    template <typename T>
    T* Context::get() const
    {
        return static_cast<T*>(m_instances.at(typeid(T)));
    }

    template <typename T>
    bool Context::has() const
    {
        return m_instances.contains(typeid(T));
    }
} // namespace p5
