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

    private:
        std::unique_ptr<Graphics> m_graphics;
        std::shared_ptr<Framebuffer> m_defaultFramebuffer;
    };
} // namespace p5
