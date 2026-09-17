#pragma once

#include <p5cpp/p5cpp.hpp>

#include <webgpu/webgpu.h>

#include <map>
#include <utility>

namespace p5
{
    class GpuDevice;

    // Caches WGPUSampler objects keyed by (filter, wrap). Only 6 possible combinations exist, so
    // no eviction is needed -- purely a single-responsibility extraction out of Renderer.
    class SamplerCache
    {
    public:
        explicit SamplerCache(GpuDevice& gpuDevice);
        ~SamplerCache();

        SamplerCache(const SamplerCache&) = delete;
        SamplerCache& operator=(const SamplerCache&) = delete;

        WGPUSampler getOrCreate(TextureFilter filter, TextureWrap wrap);

    private:
        GpuDevice& m_gpuDevice;
        std::map<std::pair<TextureFilter, TextureWrap>, WGPUSampler> m_samplers;
    };
} // namespace p5
