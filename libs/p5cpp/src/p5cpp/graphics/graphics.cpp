#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/graphics_impl.hpp>
#include <p5cpp/graphics/texture_impl.hpp>
#include <p5cpp/graphics/gpu_device.hpp>

#include <webgpu/webgpu.h>

namespace p5
{
    GraphicsImpl::~GraphicsImpl()
    {
        if (msaaView != nullptr) {
            wgpuTextureViewRelease(msaaView);
        }
        if (msaaTexture != nullptr) {
            wgpuTextureDestroy(msaaTexture);
            wgpuTextureRelease(msaaTexture);
        }
    }

    bool Graphics::isValid() const
    {
        return impl != nullptr;
    }

    std::optional<Graphics> createGraphics(uint32_t width, uint32_t height, uint32_t samples)
    {
        std::optional<Texture> colorTexture = loadTexture(width, height, {});
        if (not colorTexture.has_value()) {
            return std::nullopt;
        }

        auto impl = std::make_shared<GraphicsImpl>();

        if (samples >= 2) {
            constexpr uint32_t sampleCount = 4;
            if (samples != sampleCount) {
                warn("createGraphics() clamped samples from {} to {} -- WebGPU only guarantees support for 4x MSAA", samples, sampleCount);
            }

            GpuDevice& gpuDevice = requireDependency<GpuDevice>();

            const std::optional<WGPUTextureFormat> colorFormat = toWGPUTextureFormat(colorTexture->pixelFormat);

            WGPUTextureDescriptor msaaDesc {};
            msaaDesc.dimension = WGPUTextureDimension_2D;
            msaaDesc.size = WGPUExtent3D {width, height, 1};
            msaaDesc.format = *colorFormat;
            msaaDesc.mipLevelCount = 1;
            msaaDesc.sampleCount = sampleCount;
            msaaDesc.usage = WGPUTextureUsage_RenderAttachment;

            WGPUTexture msaaTexture = wgpuDeviceCreateTexture(gpuDevice.getDevice(), &msaaDesc);
            if (msaaTexture == nullptr) {
                error("createGraphics() failed to create a multisampled texture; continuing without antialiasing");
            } else {
                impl->msaaTexture = msaaTexture;
                impl->msaaView = wgpuTextureCreateView(msaaTexture, nullptr);
            }
        }

        return Graphics {
            .impl = std::move(impl),
            .colorTexture = std::move(colorTexture).value(),
            .size = uint2 {.x = width, .y = height},
        };
    }

    Pixels Graphics::loadPixels() const
    {
        return colorTexture.loadPixels();
    }

    void Graphics::updatePixels(const Pixels& pixels)
    {
        colorTexture.updatePixels(pixels);
    }
} // namespace p5
