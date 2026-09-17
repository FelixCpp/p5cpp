#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/canvas.hpp>
#include <p5cpp/graphics/gpu_device.hpp>
#include <p5cpp/graphics/blit_pass.hpp>

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
        void withDefaultGraphics(const Next& next);
        void handleWindowResize(uint32_t width, uint32_t height);
        void handleFramebufferResize(uint32_t width, uint32_t height);
        void recreateDefaultGraphics();

        std::unique_ptr<GpuDevice> m_gpuDevice;
        std::unique_ptr<BlitPass> m_blitPass;
        std::unique_ptr<Canvas> m_canvas;
        Graphics m_defaultGraphics;
        uint2 m_size;
        uint32_t m_samples;
    };
} // namespace p5
