#include <p5cpp/application/frame_module.hpp>
#include <p5cpp/application/logging.hpp>
#include <p5cpp/application/app_context.hpp>

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
            m_frameComponent.close();
        }

        next();
    }

    void FrameModule::draw(AppContext& context, Next next)
    {
        m_frameComponent.update();

        if (m_frameComponent.isLooping()) {
            next();
        }
    }

    void FrameModule::destroy(AppContext& context, Next next)
    {
        next();
        context.unregisterService<FrameComponent>();
    }
} // namespace p5cpp
