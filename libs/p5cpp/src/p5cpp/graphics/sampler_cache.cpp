#include <p5cpp/graphics/sampler_cache.hpp>
#include <p5cpp/graphics/gpu_device.hpp>

namespace p5
{
    namespace
    {
        WGPUFilterMode toWgpu(TextureFilter filter)
        {
            switch (filter) {
                case TextureFilter::nearest: return WGPUFilterMode_Nearest;
                case TextureFilter::linear: return WGPUFilterMode_Linear;
                default:
                    error("SamplerCache: invalid TextureFilter, falling back to nearest");
                    return WGPUFilterMode_Nearest;
            }
        }

        WGPUAddressMode toWgpu(TextureWrap wrap)
        {
            switch (wrap) {
                case TextureWrap::repeat: return WGPUAddressMode_Repeat;
                case TextureWrap::mirroredRepeat: return WGPUAddressMode_MirrorRepeat;
                case TextureWrap::clampToEdge: return WGPUAddressMode_ClampToEdge;
                default:
                    error("SamplerCache: invalid TextureWrap, falling back to clampToEdge");
                    return WGPUAddressMode_ClampToEdge;
            }
        }
    } // namespace

    SamplerCache::SamplerCache(GpuDevice& gpuDevice)
        : m_gpuDevice(gpuDevice)
    {
    }

    SamplerCache::~SamplerCache()
    {
        for (auto& [key, sampler] : m_samplers) {
            wgpuSamplerRelease(sampler);
        }
    }

    WGPUSampler SamplerCache::getOrCreate(TextureFilter filter, TextureWrap wrap)
    {
        const auto key = std::make_pair(filter, wrap);
        if (const auto it = m_samplers.find(key); it != m_samplers.end()) {
            return it->second;
        }

        WGPUSamplerDescriptor desc {};
        desc.addressModeU = toWgpu(wrap);
        desc.addressModeV = toWgpu(wrap);
        desc.magFilter = toWgpu(filter);
        desc.minFilter = toWgpu(filter);
        desc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
        desc.maxAnisotropy = 1;

        WGPUSampler sampler = wgpuDeviceCreateSampler(m_gpuDevice.getDevice(), &desc);

        m_samplers[key] = sampler;
        return sampler;
    }
} // namespace p5
