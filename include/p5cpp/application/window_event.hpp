#pragma once

namespace p5cpp
{
    enum class MouseButton {
        left,
        middle,
        right,
    };

    // clang-format off
    enum class Key
    {
        unknown,

        // Printable characters
        space,
        apostrophe, comma, minus, period, slash,
        n0, n1, n2, n3, n4, n5, n6, n7, n8, n9,
        semicolon, equal,
        leftBracket, backslash, rightBracket, graveAccent,

        // Letters
        a, b, c, d, e, f, g, h, i, j, k, l, m,
        n, o, p, q, r, s, t, u, v, w, x, y, z,

        // Control & navigation
        escape, enter, tab, backspace,
        insert, del,
        right, left, down, up,
        pageUp, pageDown, home, end,

        // Function keys
        f1,  f2,  f3,  f4,  f5,  f6,
        f7,  f8,  f9,  f10, f11, f12,

        // Modifier keys
        leftShift,  leftCtrl,  leftAlt,  leftSuper,
        rightShift, rightCtrl, rightAlt, rightSuper,
    };
    // clang-format on

    namespace KeyMod
    {
        inline constexpr int shift = 0x01;
        inline constexpr int ctrl = 0x02;
        inline constexpr int alt = 0x04;
        inline constexpr int super = 0x08;
    } // namespace KeyMod

    enum class EventType {
        close,
        mouseMove,
        mouseDrag,
        mousePress,
        mouseRelease,
        mouseScroll,
        keyPress,
        keyRelease,
        keyRepeat,
        character,
        windowResize,
        framebufferResize,
        fileDrop,
    };

    struct WindowEvent
    {
        EventType type;

        struct MouseMoveData
        {
            int x, y;
        };

        struct MouseButtonData
        {
            MouseButton button;
            int x, y;
        };

        struct MouseScrollData
        {
            float dx, dy;
            int x, y;
        };

        struct KeyData
        {
            Key key;
            int mods;
        };

        struct CharData
        {
            char32_t codepoint;
        };

        struct ResizeData
        {
            int width, height;
        };

        // Non-owning: paths point into memory owned by the platform layer and are
        // only valid for the duration of the event dispatch that carries them.
        struct FileDropData
        {
            const char* const* paths;
            int count;
        };

        union {
            MouseMoveData mouseMove; // used for both mouseMove and mouseDrag
            MouseButtonData mouseButton; // used for both mousePress and mouseRelease
            MouseScrollData mouseScroll;
            KeyData keyEvent; // used for keyPress, keyRelease and keyRepeat
            CharData charEvent;
            ResizeData windowResize;
            ResizeData framebufferResize;
            FileDropData fileDrop;
        };
    };
} // namespace p5cpp
