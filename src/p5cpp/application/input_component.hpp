#pragma once

#include <p5cpp/application/window_event.hpp>

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

    private:
        int mouseX;
        int mouseY;
        int pmouseX;
        int pmouseY;

        int physicalWidth;
        int physicalHeight;
        int logicalWidth;
        int logicalHeight;
    };
} // namespace p5cpp
