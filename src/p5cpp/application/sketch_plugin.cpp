#include <p5cpp/application/sketch_plugin.hpp>

namespace p5
{
    void SketchPlugin::setup(Context& context, const Next& next)
    {
        m_sketch = createSketch();
        context.provide(m_sketch.get());

        m_sketch->setup();

        next();
    }

    void SketchPlugin::event(Context& context, const Next& next, const WindowEvent& event)
    {
        if (m_sketch != nullptr) {
            m_sketch->event(event);
        }

        next();
    }

    void SketchPlugin::draw(Context& context, const Next& next)
    {
        if (m_sketch != nullptr) {
            m_sketch->draw();
        }

        next();
    }

    void SketchPlugin::destroy(Context& context, const Next& next)
    {
        next();

        if (m_sketch != nullptr) {
            m_sketch->destroy();
            m_sketch.reset();
        }
    }
} // namespace p5
