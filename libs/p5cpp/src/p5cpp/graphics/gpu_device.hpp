#pragma once

#include <p5cpp/p5cpp.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <webgpu/webgpu.h>

#include <memory>

namespace p5
{
    class Window;

    class GpuDevice
    {
    public:
        static std::unique_ptr<GpuDevice> create(Window& window);
        ~GpuDevice();

        WGPUDevice getDevice() const;
        WGPUQueue getQueue() const;
        WGPUTextureFormat getSurfaceFormat() const;

        void reconfigure(uint32_t width, uint32_t height);

        WGPUTexture acquireNextSurfaceTexture();

        void present();

    private:
        explicit GpuDevice(GLFWwindow* window, WGPUInstance instance, WGPUAdapter adapter, WGPUDevice device, WGPUQueue queue, WGPUSurface surface, WGPUTextureFormat surfaceFormat);

        GLFWwindow* m_window;

        WGPUInstance m_instance;
        WGPUAdapter m_adapter;
        WGPUDevice m_device;
        WGPUQueue m_queue;
        WGPUSurface m_surface;
        WGPUTextureFormat m_surfaceFormat;

        WGPUTexture m_acquiredSurfaceTexture = nullptr;

        uint32_t m_configuredWidth = 0;
        uint32_t m_configuredHeight = 0;
    };
} // namespace p5
