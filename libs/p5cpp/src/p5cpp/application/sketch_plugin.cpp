#include <p5cpp/application/sketch_plugin.hpp>
#include <p5cpp/application/kernel.hpp>

namespace p5
{
    void SketchLoaderPlugin::setup(Context& context, const Next& next)
    {
        std::unique_ptr<Sketch> sketch = createSketch();

        std::vector<std::unique_ptr<Plugin>> plugins = sketch->plugins();
        plugins.push_back(std::make_unique<SketchPlugin>(std::move(sketch)));

        getKernel().addPluginsAndSetup(std::move(plugins));

        next();
    }

    SketchPlugin::SketchPlugin(std::unique_ptr<Sketch> sketch)
        : m_sketch(std::move(sketch))
    {
    }

    void SketchPlugin::setup(Context& context, const Next& next)
    {
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
