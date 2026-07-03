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
} // namespace p5cpp
