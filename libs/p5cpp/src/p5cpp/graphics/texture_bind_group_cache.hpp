#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/sampler_cache.hpp>

#include <webgpu/webgpu.h>

#include <map>

namespace p5
{
    class GpuDevice;

    class TextureBindGroupCache
    {
    public:
        TextureBindGroupCache(GpuDevice& gpuDevice, WGPUBindGroupLayout textureLayout);
        ~TextureBindGroupCache();

        TextureBindGroupCache(const TextureBindGroupCache&) = delete;
        TextureBindGroupCache& operator=(const TextureBindGroupCache&) = delete;

        WGPUBindGroup getOrCreate(const Texture& texture, TextureFilter filter, TextureWrap wrap);
        void evictUnused();

    private:
        struct Key
        {
            TextureImpl* texture;
            TextureFilter filter;
            TextureWrap wrap;

            bool operator<(const Key& other) const;
        };
        struct Entry
        {
            Texture texture;
            WGPUBindGroup bindGroup;
        };

        GpuDevice& m_gpuDevice;
        WGPUBindGroupLayout m_textureLayout;
        SamplerCache m_samplerCache;
        std::map<Key, Entry> m_bindGroups;
    };
} // namespace p5
