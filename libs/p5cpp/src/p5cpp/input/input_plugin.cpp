#include <p5cpp/input/input_plugin.hpp>

namespace p5
{
    void InputPlugin::setup(const Next& next)
    {
        m_input = std::make_unique<Input>();
        provideDependency(m_input.get());

        next();
    }

    void InputPlugin::event(const Next& next, const WindowEvent& event)
    {
        m_input->process(event);

        next();
    }

    void InputPlugin::draw(const Next& next)
    {
        next();
        m_input->reset();
    }

    void InputPlugin::destroy(const Next& next)
    {
        next();
        removeDependency<Input>();
    }
} // namespace p5
