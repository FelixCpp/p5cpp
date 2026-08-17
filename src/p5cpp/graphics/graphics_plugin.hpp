#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/graphics.hpp>

namespace p5
{
    class GraphicsPlugin : public Plugin
    {
    public:
        void setup(Context& context, const Next& next);
        void event(Context& context, const Next& next, const WindowEvent& event);
        void draw(Context& context, const Next& next);
        void destroy(Context& context, const Next& next);

        // samples >= 2 requests MSAA antialiasing for the default (screen) canvas; recreates it
        // immediately at the current size, so calling this from setup()/draw() takes effect right
        // away rather than waiting for the next resize.
        void smooth(uint32_t samples = 4);
        void noSmooth();

    private:
        void recreateDefaultFramebuffer();

        std::unique_ptr<Graphics> m_graphics;
        std::shared_ptr<Framebuffer> m_defaultFramebuffer;
        uint2 m_size {0, 0};
        uint32_t m_samples = 0;
    };
} // namespace p5
