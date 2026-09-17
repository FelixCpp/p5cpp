#pragma once

#include <p5cpp/p5cpp.hpp>

#include <webgpu/webgpu.h>

#include <map>

namespace p5
{
    class GpuDevice;

    class BlitPass
    {
    public:
        explicit BlitPass(GpuDevice& gpuDevice);
        ~BlitPass();

        BlitPass(const BlitPass&) = delete;
        BlitPass& operator=(const BlitPass&) = delete;

        void blit(const Graphics& graphics);

    private:
        WGPURenderPipeline getOrCreatePipeline(WGPUTextureFormat targetFormat);

        GpuDevice& m_gpuDevice;
        WGPUBindGroupLayout m_bindGroupLayout;
        WGPUPipelineLayout m_pipelineLayout;
        WGPUShaderModule m_vertexModule;
        WGPUShaderModule m_fragmentModule;
        WGPUSampler m_sampler;
        std::map<WGPUTextureFormat, WGPURenderPipeline> m_pipelines;
    };
} // namespace p5
