#include "sketch_module.hpp"

namespace p5cpp
{
    void SketchModule::setup(AppContext& context, Next next)
    {
        info("SketchModule setup");
        sketch = createSketch();
        sketch->setup();
        next();
    }

    void SketchModule::event(AppContext& context, WindowEvent& event, Next next)
    {
        sketch->event(event);
        next();
    }

    void SketchModule::draw(AppContext& context, Next next)
    {
        sketch->draw();
        next();
    }

    void SketchModule::destroy(AppContext& context, Next next)
    {
        next();

        sketch->destroy();
        sketch.reset();
    }
} // namespace p5cpp
