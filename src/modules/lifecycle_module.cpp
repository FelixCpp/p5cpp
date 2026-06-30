#include "lifecycle_module.hpp"
#include "../app_context.hpp"

namespace p5cpp
{
    void LifecycleModule::event(AppContext& context, WindowEvent& event, Next next)
    {
        if (event.type == EventType::close) {
            context.lifecycleInfo.closeRequested = true;
        }

        next();
    }

    void LifecycleModule::draw(AppContext& context, Next next)
    {
        if (not context.lifecycleInfo.isPaused) {
            next();
        }
    }
} // namespace p5cpp
