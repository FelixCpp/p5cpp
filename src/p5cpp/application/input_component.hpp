#pragma once

#include <p5cpp/application/window_event.hpp>

#include <filesystem>
#include <span>
#include <unordered_set>
#include <vector>

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
        void addScrollDelta(float dx, float dy);
        void addCharTyped(char32_t codepoint);
        void addDroppedFiles(const char* const* paths, int count);

        // Clears the per-frame "pressed"/"released" edge state, the accumulated
        // scroll delta, and the typed-character/dropped-file queues. Called once
        // per frame after the draw chain has run, so this state stays visible for
        // the whole frame in which it occurred.
        void clearFrameState();

        bool isKeyDown(Key key) const;
        bool isKeyPressed(Key key) const;
        bool isKeyReleased(Key key) const;

        bool isMouseDown(MouseButton button) const;
        bool isMousePressed(MouseButton button) const;
        bool isMouseReleased(MouseButton button) const;

        // True while any/a given mouse button is held down *and* the mouse
        // position changed since the last mouse-move sample.
        bool isMouseDragging() const;
        bool isMouseDragging(MouseButton button) const;

        float getScrollX() const;
        float getScrollY() const;

        std::span<const char32_t> getCharsTyped() const;
        std::span<const std::filesystem::path> getDroppedFiles() const;

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

        float scrollX;
        float scrollY;

        std::vector<char32_t> charsTyped;
        std::vector<std::filesystem::path> droppedFiles;
    };
} // namespace p5cpp
