#include "input_module.hpp"
#include "../app_context.hpp"
#include "../services/window.hpp"

namespace p5cpp
{
    void InputModule::setup(AppContext& context, Next next)
    {
        info("InputModule setup");

        const int2 mousePosition = context.window->getMousePosition();
        context.inputInfo.mouseX = mousePosition.x;
        context.inputInfo.mouseY = mousePosition.y;
        context.inputInfo.pmouseX = mousePosition.x;
        context.inputInfo.pmouseY = mousePosition.y;

        const int2 logicalSize = context.window->getLogicalSize();
        context.inputInfo.logicalWidth = logicalSize.x;
        context.inputInfo.logicalHeight = logicalSize.y;

        const int2 physicalSize = context.window->getPhysicalSize();
        context.inputInfo.physicalWidth = physicalSize.x;
        context.inputInfo.physicalHeight = physicalSize.y;

        next();
    }

    void InputModule::event(AppContext& context, WindowEvent& event, Next next)
    {
        switch (event.type) {
            case EventType::mouseMove:
                context.inputInfo.pmouseX = context.inputInfo.mouseX;
                context.inputInfo.pmouseY = context.inputInfo.mouseY;
                context.inputInfo.mouseX = event.mouseMove.x;
                context.inputInfo.mouseY = event.mouseMove.y;
                break;

            case EventType::windowResize:
                context.inputInfo.logicalWidth = event.windowResize.width;
                context.inputInfo.logicalHeight = event.windowResize.height;
                break;

            case EventType::framebufferResize:
                context.inputInfo.physicalWidth = event.framebufferResize.width;
                context.inputInfo.physicalHeight = event.framebufferResize.height;
                break;

            default:
                break;
        }

        next();
    }

} // namespace p5cpp
