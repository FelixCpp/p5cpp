#pragma once

#include <p5cpp/p5cpp.hpp>

namespace p5
{
    class SketchPlugin : public Plugin
    {
    public:
        void setup(Context& context, const Next& next);
        void event(Context& context, const Next& next, const WindowEvent& event);
        void draw(Context& context, const Next& next);
        void destroy(Context& context, const Next& next);

    private:
        std::unique_ptr<Sketch> m_sketch;
    };
} // namespace p5
