#include "sketch_module.hpp"
#include "../../app_context.hpp"

namespace p5cpp
{
    void SketchModule::setup(AppContext& context, Next next)
    {
        info("SketchModule setup");

        sketch = createSketch();
        sketch->setup();

        context.registerService(sketch.get());
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

        context.unregisterService<Sketch>();

        sketch->destroy();
        sketch.reset();
    }
} // namespace p5cpp
