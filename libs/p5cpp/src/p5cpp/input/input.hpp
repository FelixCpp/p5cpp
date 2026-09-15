#pragma once

#include <p5cpp/p5cpp.hpp>

#include <array>
#include <span>
#include <string>
#include <vector>

namespace p5
{
    class Input
    {
    public:
        void process(const WindowEvent& event);
        void reset();

        bool isKeyDown(Key key) const;
        bool isKeyPressed(Key key) const;
        bool isKeyReleased(Key key) const;

        bool isMouseButtonDown(MouseButton button) const;
        bool isMouseButtonPressed(MouseButton button) const;
        bool isMouseButtonReleased(MouseButton button) const;

        double mouseX() const;
        double mouseY() const;
        double previousMouseX() const;
        double previousMouseY() const;

        double scrollX() const;
        double scrollY() const;

        bool isSwiped(MouseButton button) const;
        SwipeDirection swipeDirection(MouseButton button) const;
        double swipeDistance(MouseButton button) const;
        double swipeDuration(MouseButton button) const;
        void setSwipeThreshold(double minDistance, double maxDurationInSeconds);

        bool cursorInWindow() const;

        std::span<const std::string> droppedFiles() const;

        std::span<const uint32_t> typedChars() const;

    private:
        std::array<bool, static_cast<size_t>(Key::Count)> m_keyDown {};
        std::array<bool, static_cast<size_t>(Key::Count)> m_keyPressed {};
        std::array<bool, static_cast<size_t>(Key::Count)> m_keyReleased {};

        std::array<bool, static_cast<size_t>(MouseButton::count)> m_buttonDown {};
        std::array<bool, static_cast<size_t>(MouseButton::count)> m_buttonPressed {};
        std::array<bool, static_cast<size_t>(MouseButton::count)> m_buttonReleased {};

        double m_mouseX = 0.0;
        double m_mouseY = 0.0;
        double m_prevMouseX = 0.0;
        double m_prevMouseY = 0.0;

        double m_scrollX = 0.0;
        double m_scrollY = 0.0;

        std::array<bool, static_cast<size_t>(MouseButton::count)> m_dragging {};
        std::array<double, static_cast<size_t>(MouseButton::count)> m_dragStartX {};
        std::array<double, static_cast<size_t>(MouseButton::count)> m_dragStartY {};
        std::array<double, static_cast<size_t>(MouseButton::count)> m_dragStartTime {};

        std::array<bool, static_cast<size_t>(MouseButton::count)> m_swiped {};
        std::array<SwipeDirection, static_cast<size_t>(MouseButton::count)> m_swipeDirection {};
        std::array<double, static_cast<size_t>(MouseButton::count)> m_swipeDistance {};
        std::array<double, static_cast<size_t>(MouseButton::count)> m_swipeDuration {};

        double m_swipeMinDistance = 50.0;
        double m_swipeMaxDuration = 0.5;

        bool m_cursorInWindow = false;

        std::vector<std::string> m_droppedFiles;
        std::vector<uint32_t> m_typedChars;
    };
} // namespace p5
