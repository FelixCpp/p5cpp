#ifndef P5CPP_SKETCH_PLUGIN_HPP
#define P5CPP_SKETCH_PLUGIN_HPP

#include <p5cpp/p5cpp.hpp>

namespace p5
{
    class SketchPlugin : public Plugin
    {
    public:
        void setup(PluginPipeline& pipeline);
        void event(PluginPipeline& pipeline, const WindowEvent& event);
        void draw(PluginPipeline& pipeline);
        void destroy(PluginPipeline& pipeline);

    private:
        std::unique_ptr<Sketch> sketch;
    };
} // namespace p5

#endif // P5CPP_SKETCH_PLUGIN_HPP
