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

    class SketchLoaderPlugin : public Plugin
    {
    public:
        explicit SketchLoaderPlugin(SketchFactory sketchFactory);

        void setup(Context& context, const Next& next);

    private:
        SketchFactory m_sketchFactory;
    };
} // namespace p5
