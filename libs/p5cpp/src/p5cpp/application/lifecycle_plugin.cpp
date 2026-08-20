#include <p5cpp/application/lifecycle_plugin.hpp>

namespace p5
{
    LifecyclePlugin::LifecyclePlugin()
        : m_lifecycle()
    {
    }

    void LifecyclePlugin::setup(Context& context, const Next& next)
    {
        m_lifecycle = Lifecycle();
        context.provide(&m_lifecycle);

        next();
    }

    void LifecyclePlugin::event(Context& context, const Next& next, const WindowEvent& event)
    {
        if (event.is<WindowEvent::Close>()) {
            m_lifecycle.close();
        }

        if (event.is<WindowEvent::KeyPress>()) {
            const auto& keyPress = event.as<WindowEvent::KeyPress>();

            if (keyPress.key == Key::Escape) {
                m_lifecycle.close();
            }
        }

        next();
    }

    void LifecyclePlugin::draw(Context& context, const Next& next)
    {
        next();
    }

    void LifecyclePlugin::destroy(Context& context, const Next& next)
    {
        next();
        context.remove<Lifecycle>();
    }
} // namespace p5
