#include <p5cpp/application/sketch_plugin.hpp>
#include <p5cpp/application/kernel.hpp>

namespace p5
{
    SketchLoaderPlugin::SketchLoaderPlugin(SketchFactory sketchFactory)
        : m_sketchFactory(std::move(sketchFactory))
    {
    }

    void SketchLoaderPlugin::setup([[maybe_unused]] Context& context, [[maybe_unused]] const Next& next)
    {
        std::unique_ptr<Sketch> sketch = m_sketchFactory();

        std::vector<std::unique_ptr<Plugin>> plugins;
        plugins.push_back(std::make_unique<SketchPlugin>(std::move(sketch)));
        getKernel().addPluginsAndSetup(std::move(plugins));
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

    void SketchPlugin::event([[maybe_unused]] Context& context, const Next& next, const WindowEvent& event)
    {
        if (m_sketch != nullptr) {
            m_sketch->event(event);
        }

        next();
    }

    void SketchPlugin::draw([[maybe_unused]] Context& context, const Next& next)
    {
        if (m_sketch != nullptr) {
            m_sketch->draw();
        }

        next();
    }

    void SketchPlugin::destroy([[maybe_unused]] Context& context, const Next& next)
    {
        next();

        if (m_sketch != nullptr) {
            m_sketch->destroy();
            m_sketch.reset();
        }
    }
} // namespace p5
