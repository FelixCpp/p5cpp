#include <p5cpp/input/input_plugin.hpp>

namespace p5
{
    void InputPlugin::setup(Context& context, const Next& next)
    {
        context.provide(&m_input);

        next();
    }

    void InputPlugin::event(Context& context, const Next& next, const WindowEvent& event)
    {
        m_input.onEvent(event);

        next();
    }

    void InputPlugin::draw(Context& context, const Next& next)
    {
        next();

        m_input.endFrame();
    }

    void InputPlugin::destroy(Context& context, const Next& next)
    {
        next();

        context.remove<Input>();
    }
} // namespace p5
