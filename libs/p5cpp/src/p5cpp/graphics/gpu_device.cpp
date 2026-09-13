#include <p5cpp/graphics/gpu_device.hpp>
#include <p5cpp/window/window.hpp>

#include <glfw3webgpu.h>

#include <webgpu/wgpu.h>

#include <atomic>
#include <iostream>

namespace p5
{
    static WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPUSurface compatibleSurface)
    {
        WGPUAdapter adapter = nullptr;
        std::atomic<bool> done {false};

        WGPURequestAdapterOptions options {};
        options.compatibleSurface = compatibleSurface;
        options.powerPreference = WGPUPowerPreference_HighPerformance;

        WGPURequestAdapterCallbackInfo callbackInfo {};
        callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        callbackInfo.callback = [](WGPURequestAdapterStatus status, WGPUAdapter result, WGPUStringView message, void* userdata1, void* userdata2) {
            if (status != WGPURequestAdapterStatus_Success) {
                std::cerr << "wgpuInstanceRequestAdapter failed: " << std::string_view(message.data, message.length) << std::endl;
            }
            *static_cast<WGPUAdapter*>(userdata1) = result;
            static_cast<std::atomic<bool>*>(userdata2)->store(true);
        };
        callbackInfo.userdata1 = &adapter;
        callbackInfo.userdata2 = &done;

        wgpuInstanceRequestAdapter(instance, &options, callbackInfo);
        while (not done.load()) {
            wgpuInstanceProcessEvents(instance);
        }

        return adapter;
    }

    static WGPUDevice requestDeviceSync(WGPUInstance instance, WGPUAdapter adapter)
    {
        WGPUDevice device = nullptr;
        std::atomic<bool> done {false};

        WGPUDeviceDescriptor descriptor {};
        descriptor.uncapturedErrorCallbackInfo.callback = [](WGPUDevice const*, WGPUErrorType type, WGPUStringView message, void*, void*) {
            std::cerr << "WebGPU uncaptured error (" << static_cast<int>(type) << "): " << std::string_view(message.data, message.length) << std::endl;
        };

        WGPURequestDeviceCallbackInfo callbackInfo {};
        callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        callbackInfo.callback = [](WGPURequestDeviceStatus status, WGPUDevice result, WGPUStringView message, void* userdata1, void* userdata2) {
            if (status != WGPURequestDeviceStatus_Success) {
                std::cerr << "wgpuAdapterRequestDevice failed: " << std::string_view(message.data, message.length) << std::endl;
            }
            *static_cast<WGPUDevice*>(userdata1) = result;
            static_cast<std::atomic<bool>*>(userdata2)->store(true);
        };
        callbackInfo.userdata1 = &device;
        callbackInfo.userdata2 = &done;

        wgpuAdapterRequestDevice(adapter, &descriptor, callbackInfo);
        while (not done.load()) {
            wgpuInstanceProcessEvents(instance);
        }

        return device;
    }

    std::unique_ptr<GpuDevice> GpuDevice::create(Window& window)
    {
        GLFWwindow* handle = window.getHandle();

        WGPUInstance instance = wgpuCreateInstance(nullptr);
        if (instance == nullptr) {
            return nullptr;
        }

        WGPUSurface surface = glfwCreateWindowWGPUSurface(instance, handle);
        if (surface == nullptr) {
            wgpuInstanceRelease(instance);
            return nullptr;
        }

        WGPUAdapter adapter = requestAdapterSync(instance, surface);
        if (adapter == nullptr) {
            wgpuSurfaceRelease(surface);
            wgpuInstanceRelease(instance);
            return nullptr;
        }

        WGPUDevice device = requestDeviceSync(instance, adapter);
        if (device == nullptr) {
            wgpuAdapterRelease(adapter);
            wgpuSurfaceRelease(surface);
            wgpuInstanceRelease(instance);
            return nullptr;
        }

        WGPUQueue queue = wgpuDeviceGetQueue(device);

        WGPUSurfaceCapabilities capabilities {};
        wgpuSurfaceGetCapabilities(surface, adapter, &capabilities);
        WGPUTextureFormat surfaceFormat = capabilities.formatCount > 0 ? capabilities.formats[0] : WGPUTextureFormat_BGRA8Unorm;
        for (size_t i = 0; i < capabilities.formatCount; ++i) {
            const WGPUTextureFormat format = capabilities.formats[i];
            if (format != WGPUTextureFormat_BGRA8UnormSrgb and format != WGPUTextureFormat_RGBA8UnormSrgb) {
                surfaceFormat = format;
                break;
            }
        }
        wgpuSurfaceCapabilitiesFreeMembers(capabilities);

        auto gpuDevice = std::unique_ptr<GpuDevice>(new GpuDevice {handle, instance, adapter, device, queue, surface, surfaceFormat});

        const uint2 physicalSize = window.getPhysicalSize();
        gpuDevice->reconfigure(physicalSize.x, physicalSize.y);

        return gpuDevice;
    }

    GpuDevice::GpuDevice(GLFWwindow* window, WGPUInstance instance, WGPUAdapter adapter, WGPUDevice device, WGPUQueue queue, WGPUSurface surface, WGPUTextureFormat surfaceFormat)
        : m_window(window),
          m_instance(instance),
          m_adapter(adapter),
          m_device(device),
          m_queue(queue),
          m_surface(surface),
          m_surfaceFormat(surfaceFormat)
    {
    }

    GpuDevice::~GpuDevice()
    {
        if (m_acquiredSurfaceTexture != nullptr) {
            wgpuTextureRelease(m_acquiredSurfaceTexture);
        }

        wgpuSurfaceUnconfigure(m_surface);
        wgpuSurfaceRelease(m_surface);

        wgpuQueueRelease(m_queue);
        wgpuDeviceRelease(m_device);
        wgpuAdapterRelease(m_adapter);
        wgpuInstanceRelease(m_instance);
    }

    WGPUDevice GpuDevice::getDevice() const
    {
        return m_device;
    }

    WGPUQueue GpuDevice::getQueue() const
    {
        return m_queue;
    }

    WGPUTextureFormat GpuDevice::getSurfaceFormat() const
    {
        return m_surfaceFormat;
    }

    void GpuDevice::reconfigure(uint32_t width, uint32_t height)
    {
        if (width == 0 or height == 0) {
            return;
        }

        glfwUpdateWGPUSurfaceGeometry(m_window);

        WGPUSurfaceConfiguration config {};
        config.device = m_device;
        config.format = m_surfaceFormat;
        config.usage = WGPUTextureUsage_RenderAttachment;
        config.width = width;
        config.height = height;
        config.presentMode = WGPUPresentMode_Fifo;
        config.alphaMode = WGPUCompositeAlphaMode_Auto;
        wgpuSurfaceConfigure(m_surface, &config);

        m_configuredWidth = width;
        m_configuredHeight = height;
    }

    WGPUTexture GpuDevice::acquireNextSurfaceTexture()
    {
        if (m_acquiredSurfaceTexture != nullptr) {
            return m_acquiredSurfaceTexture;
        }

        WGPUSurfaceTexture surfaceTexture {};
        wgpuSurfaceGetCurrentTexture(m_surface, &surfaceTexture);

        if (surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_Outdated ||
            surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_Lost) {
            reconfigure(m_configuredWidth, m_configuredHeight);
            wgpuSurfaceGetCurrentTexture(m_surface, &surfaceTexture);
        }

        if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
            surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
            return nullptr;
        }

        m_acquiredSurfaceTexture = surfaceTexture.texture;
        return m_acquiredSurfaceTexture;
    }

    void GpuDevice::present()
    {
        if (m_acquiredSurfaceTexture == nullptr) {
            return;
        }

        wgpuSurfacePresent(m_surface);
        wgpuTextureRelease(m_acquiredSurfaceTexture);
        m_acquiredSurfaceTexture = nullptr;
    }
} // namespace p5
