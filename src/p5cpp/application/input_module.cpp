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

            case EventType::keyPress:
                m_inputComponent.setKeyDown(event.keyEvent.key, true);
                break;

            case EventType::keyRelease:
                m_inputComponent.setKeyDown(event.keyEvent.key, false);
                break;

            case EventType::mousePress:
                m_inputComponent.setMouseButtonDown(event.mouseButton.button, true);
                break;

            case EventType::mouseRelease:
                m_inputComponent.setMouseButtonDown(event.mouseButton.button, false);
                break;

            case EventType::mouseScroll:
                m_inputComponent.addScrollDelta(event.mouseScroll.dx, event.mouseScroll.dy);
                break;

            case EventType::character:
                m_inputComponent.addCharTyped(event.charEvent.codepoint);
                break;

            case EventType::fileDrop:
                m_inputComponent.addDroppedFiles(event.fileDrop.paths, event.fileDrop.count);
                break;

            default:
                break;
        }

        next();
    }

    void InputModule::draw(AppContext& context, Next next)
    {
        // Events for this frame (dispatched by WindowModule::draw's pollEvents(),
        // which runs before this module in the chain) are already applied to
        // m_inputComponent, so isKeyPressed()/isKeyReleased() are visible to the
        // rest of the draw chain, including the sketch's draw(). Clear the edge
        // state afterwards so it doesn't leak into the next frame.
        next();
        m_inputComponent.clearFrameState();
    }

    void InputModule::destroy(AppContext& context, Next next)
    {
        next();
        context.unregisterService<InputComponent>();
    }

} // namespace p5cpp
