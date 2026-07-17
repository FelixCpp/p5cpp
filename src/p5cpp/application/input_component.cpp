#include <p5cpp/application/input_component.hpp>

namespace p5cpp
{
    InputComponent::InputComponent()
        : mouseX(0),
          mouseY(0),
          pmouseX(0),
          pmouseY(0),
          logicalWidth(0),
          logicalHeight(0),
          physicalWidth(0),
          physicalHeight(0)
    {
    }

    void InputComponent::updateMousePosition(int x, int y)
    {
        pmouseX = mouseX;
        pmouseY = mouseY;
        mouseX = x;
        mouseY = y;
    }

    void InputComponent::updatePhysicalSize(int width, int height)
    {
        physicalWidth = width;
        physicalHeight = height;
    }

    void InputComponent::updateLogicalSize(int width, int height)
    {
        logicalWidth = width;
        logicalHeight = height;
    }

    int InputComponent::getMouseX() const { return mouseX; }
    int InputComponent::getMouseY() const { return mouseY; }
    int InputComponent::getPMouseX() const { return pmouseX; }
    int InputComponent::getPMouseY() const { return pmouseY; }
    int InputComponent::getLogicalWidth() const { return logicalWidth; }
    int InputComponent::getLogicalHeight() const { return logicalHeight; }
    int InputComponent::getPhysicalWidth() const { return physicalWidth; }
    int InputComponent::getPhysicalHeight() const { return physicalHeight; }

    void InputComponent::setKeyDown(Key key, bool down)
    {
        if (down) {
            keysDown.insert(key);
            keysPressed.insert(key);
        } else {
            keysDown.erase(key);
            keysReleased.insert(key);
        }
    }

    void InputComponent::setMouseButtonDown(MouseButton button, bool down)
    {
        if (down) {
            mouseButtonsDown.insert(button);
            mouseButtonsPressed.insert(button);
        } else {
            mouseButtonsDown.erase(button);
            mouseButtonsReleased.insert(button);
        }
    }

    void InputComponent::clearFrameState()
    {
        keysPressed.clear();
        keysReleased.clear();
        mouseButtonsPressed.clear();
        mouseButtonsReleased.clear();
    }

    bool InputComponent::isKeyDown(Key key) const { return keysDown.contains(key); }
    bool InputComponent::isKeyPressed(Key key) const { return keysPressed.contains(key); }
    bool InputComponent::isKeyReleased(Key key) const { return keysReleased.contains(key); }

    bool InputComponent::isMouseDown(MouseButton button) const { return mouseButtonsDown.contains(button); }
    bool InputComponent::isMousePressed(MouseButton button) const { return mouseButtonsPressed.contains(button); }
    bool InputComponent::isMouseReleased(MouseButton button) const { return mouseButtonsReleased.contains(button); }
} // namespace p5cpp
