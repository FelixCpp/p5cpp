#include <p5cpp/graphics/texture_bind_group_cache.hpp>
#include <p5cpp/graphics/texture_impl.hpp>
#include <p5cpp/graphics/gpu_device.hpp>

#include <tuple>

namespace p5
{
    bool TextureBindGroupCache::Key::operator<(const Key& other) const
    {
        return std::tie(texture, filter, wrap) < std::tie(other.texture, other.filter, other.wrap);
    }

    TextureBindGroupCache::TextureBindGroupCache(GpuDevice& gpuDevice, WGPUBindGroupLayout textureLayout)
        : m_gpuDevice(gpuDevice),
          m_textureLayout(textureLayout),
          m_samplerCache(gpuDevice)
    {
    }

    TextureBindGroupCache::~TextureBindGroupCache()
    {
        for (auto& [key, entry] : m_bindGroups) {
            wgpuBindGroupRelease(entry.bindGroup);
        }
    }

    WGPUBindGroup TextureBindGroupCache::getOrCreate(const Texture& texture, TextureFilter filter, TextureWrap wrap)
    {
        const Key key {texture.impl.get(), filter, wrap};
        if (const auto it = m_bindGroups.find(key); it != m_bindGroups.end()) {
            return it->second.bindGroup;
        }

        WGPUSampler sampler = m_samplerCache.getOrCreate(filter, wrap);

        WGPUBindGroupEntry entries[2] = {};
        entries[0].binding = 0;
        entries[0].textureView = texture.impl->view;
        entries[1].binding = 1;
        entries[1].sampler = sampler;

        WGPUBindGroupDescriptor desc {};
        desc.layout = m_textureLayout;
        desc.entryCount = 2;
        desc.entries = entries;

        WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(m_gpuDevice.getDevice(), &desc);

        m_bindGroups[key] = Entry {.texture = texture, .bindGroup = bindGroup};
        return bindGroup;
    }

    void TextureBindGroupCache::evictUnused()
    {
        for (auto it = m_bindGroups.begin(); it != m_bindGroups.end();) {
            if (it->second.texture.impl.use_count() == 1) {
                wgpuBindGroupRelease(it->second.bindGroup);
                it = m_bindGroups.erase(it);
            } else {
                ++it;
            }
        }
    }
} // namespace p5
