#include <p5cpp/application/lifecycle_plugin.hpp>

namespace p5
{
    LifecyclePlugin::LifecyclePlugin()
        : m_lifecycle()
    {
    }

    void LifecyclePlugin::setup(const Next& next)
    {
        m_lifecycle = std::make_unique<Lifecycle>();
        provideDependency(m_lifecycle.get());

        next();
    }

    void LifecyclePlugin::event(const Next& next, const WindowEvent& event)
    {
        if (event.is<WindowEvent::Close>()) {
            m_lifecycle->close();
        }

        if (const auto* keyPress = event.as_if<WindowEvent::KeyPress>()) {
            if (keyPress->key == Key::Escape) {
                m_lifecycle->close();
            }
        }

        next();
    }

    void LifecyclePlugin::draw(const Next& next)
    {
        next();
    }

    void LifecyclePlugin::destroy(const Next& next)
    {
        next();

        removeDependency<Lifecycle>();
        m_lifecycle.reset();
    }
} // namespace p5
