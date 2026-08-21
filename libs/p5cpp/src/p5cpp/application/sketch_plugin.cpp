#include <p5cpp/application/sketch_plugin.hpp>
#include <p5cpp/application/kernel.hpp>

namespace p5
{
    void SketchLoaderPlugin::setup(Context& context, const Next& next)
    {
        std::unique_ptr<Sketch> sketch = createSketch();

        std::vector<std::unique_ptr<Plugin>> plugins = sketch->plugins();
        plugins.push_back(std::make_unique<SketchPlugin>(std::move(sketch)));

        // addPluginsAndSetup() appends these plugins right after this one (the deque, per
        // kernel.hpp's Next-stability comment, never invalidates the outer chain's view) and
        // dispatches setup() over that whole newly-appended range itself -- which already *is* the
        // rest of the chain, since SketchLoaderPlugin is always the last plugin p5cpp.cpp registers.
        // Calling `next` afterwards would re-walk that same index range a second time: Next::operator()
        // re-checks m_index against the *current* (now-grown) deque size each call, so the index that
        // meant "past the end" before this append points squarely at what was just added. That doubled
        // every newly-added plugin's setup() -- including the user's Sketch::setup() via
        // SketchPlugin::setup() -- so this intentionally does not call next().
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
        m_setupCompleted = true;

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

        if (m_sketch != nullptr and m_setupCompleted) {
            m_sketch->destroy();
            m_sketch.reset();
        }
    }
} // namespace p5
