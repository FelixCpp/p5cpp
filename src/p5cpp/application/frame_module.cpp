#include <p5cpp/application/frame_module.hpp>
#include <p5cpp/application/logging.hpp>
#include <p5cpp/application/app_context.hpp>
#include <p5cpp/application/window.hpp>

#include <p5cpp/application/window_event.hpp>

namespace p5cpp
{
    void FrameModule::setup(AppContext& context, Next next)
    {
        info("FrameModule setup");
        context.registerService(&m_frameComponent);
        next();
    }

    void FrameModule::event(AppContext& context, WindowEvent& event, Next next)
    {
        if (event.type == EventType::close) {
            m_frameComponent.quit();
        }

        if (event.type == EventType::keyPress and event.keyEvent.key == Key::escape) {
            m_frameComponent.quit();
        }

        if (event.type == EventType::keyPress and event.keyEvent.key == Key::r and event.keyEvent.mods & KeyMod::ctrl) {
            m_frameComponent.restart();
        }

        if (event.type == EventType::keyPress and event.keyEvent.key == Key::space) {
            if (m_frameComponent.isLooping()) {
                m_frameComponent.noLoop();
            } else {
                m_frameComponent.loop();
            }
        }

        // F11 is intercepted by the OS on macOS (Mission Control), so fullscreen
        // uses the cross-platform game convention of Alt+Enter instead.
        if (event.type == EventType::keyPress and event.keyEvent.key == Key::enter and event.keyEvent.mods & KeyMod::alt) {
            Window& window = context.require<Window>();
            window.setFullscreen(not window.isFullscreen());
        }

        next();
    }

    void FrameModule::draw(AppContext& context, Next next)
    {
        m_frameComponent.update();
        next();
    }

    void FrameModule::destroy(AppContext& context, Next next)
    {
        next();
        context.unregisterService<FrameComponent>();
    }
} // namespace p5cpp
