#pragma once

#include <p5cpp/p5cpp.hpp>

namespace p5
{
    class SketchPlugin : public Plugin
    {
    public:
        explicit SketchPlugin(std::unique_ptr<Sketch> sketch);

        void setup(Context& context, const Next& next);
        void event(Context& context, const Next& next, const WindowEvent& event);
        void draw(Context& context, const Next& next);
        void destroy(Context& context, const Next& next);

    private:
        std::unique_ptr<Sketch> m_sketch;
    };

    // Statically registered as the last built-in plugin. Defers creating the user's
    // Sketch (and collecting its custom plugins) until its own setup() runs, i.e. after
    // every other built-in plugin (Window, Graphics, ...) has already completed setup,
    // since sketches commonly create GPU resources as member initializers.
    class SketchLoaderPlugin : public Plugin
    {
    public:
        void setup(Context& context, const Next& next);
    };
} // namespace p5
