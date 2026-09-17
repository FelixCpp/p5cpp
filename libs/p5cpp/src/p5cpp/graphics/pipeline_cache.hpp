#pragma once

#include <p5cpp/p5cpp.hpp>

#include <webgpu/webgpu.h>

#include <map>

namespace p5
{
    class GpuDevice;

    // Caches WGPURenderPipeline objects keyed by (shader, blend mode, sample count, color
    // format). Evicts entries whose Shader is no longer referenced anywhere else (use_count()==1
    // once the cache's own copy is discounted), so a shader that's stopped being drawn eventually
    // releases its pipelines instead of being kept alive forever; because eviction only happens
    // once nothing external references the Shader, a freed-then-reallocated ShaderImpl* can never
    // alias a still-live cache entry.
    class PipelineCache
    {
    public:
        PipelineCache(GpuDevice& gpuDevice, WGPUPipelineLayout pipelineLayout);
        ~PipelineCache();

        PipelineCache(const PipelineCache&) = delete;
        PipelineCache& operator=(const PipelineCache&) = delete;

        WGPURenderPipeline getOrCreate(const Shader& shader, const BlendMode& blendMode, uint32_t sampleCount, WGPUTextureFormat colorFormat);

        // Releases pipelines for shaders no longer referenced anywhere outside this cache. Call
        // periodically (e.g. once per flush()) rather than on every getOrCreate().
        void evictUnused();

    private:
        struct Key
        {
            ShaderImpl* shader;
            BlendMode blendMode;
            uint32_t sampleCount;
            WGPUTextureFormat colorFormat;

            bool operator<(const Key& other) const;
        };
        struct Entry
        {
            Shader shader;
            WGPURenderPipeline pipeline;
        };

        GpuDevice& m_gpuDevice;
        WGPUPipelineLayout m_pipelineLayout;
        std::map<Key, Entry> m_pipelines;
    };
} // namespace p5
