#include "input_module.hpp"

#include "../../application/app_context.hpp"

#include <p5cpp/application/window.hpp>
#include <p5cpp/application/logging.hpp>

namespace p5cpp
{
    void InputModule::setup(AppContext& context, Next next)
    {
        info("InputModule setup");

        Window& window = context.require<Window>();

        const int2 mousePosition = window.getMousePosition();
        data.mouseX = mousePosition.x;
        data.mouseY = mousePosition.y;
        data.pmouseX = mousePosition.x;
        data.pmouseY = mousePosition.y;

        const int2 logicalSize = window.getLogicalSize();
        data.logicalWidth = logicalSize.x;
        data.logicalHeight = logicalSize.y;

        const int2 physicalSize = window.getPhysicalSize();
        data.physicalWidth = physicalSize.x;
        data.physicalHeight = physicalSize.y;

        context.registerService(&data);
        next();
    }

    void InputModule::event(AppContext& context, WindowEvent& event, Next next)
    {
        switch (event.type) {
            case EventType::mouseMove:
                data.pmouseX = data.mouseX;
                data.pmouseY = data.mouseY;
                data.mouseX = event.mouseMove.x;
                data.mouseY = event.mouseMove.y;
                break;

            case EventType::windowResize:
                data.logicalWidth = event.windowResize.width;
                data.logicalHeight = event.windowResize.height;
                break;

            case EventType::framebufferResize:
                data.physicalWidth = event.framebufferResize.width;
                data.physicalHeight = event.framebufferResize.height;
                break;

            default:
                break;
        }

        next();
    }

} // namespace p5cpp
