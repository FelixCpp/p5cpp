#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/canvas.hpp>

namespace p5
{
    class GraphicsPlugin : public Plugin
    {
    public:
        GraphicsPlugin();

        void setup(const Next& next);
        void event(const Next& next, const WindowEvent& event);
        void draw(const Next& next);
        void destroy(const Next& next);

        void smooth(uint32_t samples);
        void noSmooth();

    private:
        void recreateDefaultGraphics();

        std::unique_ptr<Canvas> m_canvas;
        Graphics m_defaultGraphics;
        uint2 m_size;
        uint32_t m_samples;
    };
} // namespace p5
