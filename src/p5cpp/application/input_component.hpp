#pragma once

#include <p5cpp/application/window_event.hpp>

#include <unordered_set>

namespace p5cpp
{
    class InputComponent
    {
    public:
        InputComponent();

        void updateMousePosition(int mouseX, int mouseY);
        void updatePhysicalSize(int width, int height);
        void updateLogicalSize(int width, int height);

        int getMouseX() const;
        int getMouseY() const;
        int getPMouseX() const;
        int getPMouseY() const;

        int getPhysicalWidth() const;
        int getPhysicalHeight() const;
        int getLogicalWidth() const;
        int getLogicalHeight() const;

        void setKeyDown(Key key, bool down);
        void setMouseButtonDown(MouseButton button, bool down);

        // Clears the per-frame "pressed"/"released" edge state. Called once per
        // frame after the draw chain has run, so isKeyPressed()/isKeyReleased()
        // stay true for the whole frame in which the edge occurred.
        void clearFrameState();

        bool isKeyDown(Key key) const;
        bool isKeyPressed(Key key) const;
        bool isKeyReleased(Key key) const;

        bool isMouseDown(MouseButton button) const;
        bool isMousePressed(MouseButton button) const;
        bool isMouseReleased(MouseButton button) const;

    private:
        int mouseX;
        int mouseY;
        int pmouseX;
        int pmouseY;

        int physicalWidth;
        int physicalHeight;
        int logicalWidth;
        int logicalHeight;

        std::unordered_set<Key> keysDown;
        std::unordered_set<Key> keysPressed;
        std::unordered_set<Key> keysReleased;

        std::unordered_set<MouseButton> mouseButtonsDown;
        std::unordered_set<MouseButton> mouseButtonsPressed;
        std::unordered_set<MouseButton> mouseButtonsReleased;
    };
} // namespace p5cpp
