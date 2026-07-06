#include <p5cpp/application/input_module.hpp>

#include <p5cpp/application/window.hpp>
#include <p5cpp/application/logging.hpp>
#include <p5cpp/application/app_context.hpp>

namespace p5cpp
{
    void InputModule::setup(AppContext& context, Next next)
    {
        info("InputModule setup");

        Window& window = context.require<Window>();

        const int2 mousePosition = window.getMousePosition();
        m_inputComponent.updateMousePosition(mousePosition.x, mousePosition.y);

        const int2 logicalSize = window.getLogicalSize();
        m_inputComponent.updateLogicalSize(logicalSize.x, logicalSize.y);

        const int2 physicalSize = window.getPhysicalSize();
        m_inputComponent.updatePhysicalSize(physicalSize.x, physicalSize.y);

        context.registerService<InputComponent>(&m_inputComponent);
        next();
    }

    void InputModule::event(AppContext& context, WindowEvent& event, Next next)
    {
        switch (event.type) {
            case EventType::mouseMove:
                m_inputComponent.updateMousePosition(event.mouseMove.x, event.mouseMove.y);
                break;

            case EventType::windowResize:
                m_inputComponent.updateLogicalSize(event.windowResize.width, event.windowResize.height);
                break;

            case EventType::framebufferResize:
                m_inputComponent.updatePhysicalSize(event.framebufferResize.width, event.framebufferResize.height);
                break;

            default:
                break;
        }

        next();
    }

    void InputModule::destroy(AppContext& context, Next next)
    {
        next();
        context.unregisterService<InputComponent>();
    }

} // namespace p5cpp
