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
        m_input.process(event);

        next();
    }

    void InputPlugin::draw(Context& context, const Next& next)
    {
        next();

        m_input.reset();
    }

    void InputPlugin::destroy(Context& context, const Next& next)
    {
        next();

        context.remove<Input>();
    }
} // namespace p5
