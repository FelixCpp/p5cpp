#include <p5cpp/application/sketch_plugin.hpp>

namespace p5
{
    void SketchPlugin::setup(PluginPipeline& pipeline)
    {
        sketch = createSketch();

        if (sketch == nullptr) {
            error("No scetch was created. Please implement the createSketch() function to return a valid Sketch instance.");
            close();
        }

        if (sketch) {
            sketch->setup();
            pipeline.next();
        }
    }

    void SketchPlugin::event(PluginPipeline& pipeline, const WindowEvent& event)
    {
        if (sketch) {
            sketch->event(event);
            pipeline.next();
        }
    }

    void SketchPlugin::draw(PluginPipeline& pipeline)
    {
        if (sketch) {
            sketch->draw();
            pipeline.next();
        }
    }

    void SketchPlugin::destroy(PluginPipeline& pipeline)
    {
        if (sketch) {
            sketch->destroy();
            pipeline.next();
        }
    }
} // namespace p5
