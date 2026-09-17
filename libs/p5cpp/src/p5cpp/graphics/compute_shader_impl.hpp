#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/storage_buffer_impl.hpp>

#include <webgpu/webgpu.h>

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace p5
{
    struct ComputeBindingSlot
    {
        uint32_t binding = 0;
        bool readOnly = true;
    };

    // A cached bind group for one specific combination of bound StorageBuffers. `buffers` keeps
    // each bound StorageBuffer's shared_ptr alive so eviction can check use_count(), the same
    // lifetime-safety pattern PipelineCache/TextureBindGroupCache use.
    struct ComputeBindGroupEntry
    {
        std::vector<StorageBuffer> buffers;
        WGPUBindGroup bindGroup = nullptr;
    };

    struct ComputeShaderImpl
    {
        WGPUShaderModule module = nullptr;
        WGPUBindGroupLayout bindGroupLayout = nullptr;
        WGPUPipelineLayout pipelineLayout = nullptr;
        WGPUComputePipeline pipeline = nullptr;

        std::unordered_map<std::string, ComputeBindingSlot> bindingSlots;

        // Keyed by the bound StorageBufferImpl pointers in WGSL binding-index order, so
        // dispatchCompute() reuses the same WGPUBindGroup across frames for a particle system's
        // fixed set of buffers instead of creating and releasing one on every single call.
        std::map<std::vector<StorageBufferImpl*>, ComputeBindGroupEntry> bindGroupCache;

        ComputeShaderImpl() = default;
        ComputeShaderImpl(const ComputeShaderImpl&) = delete;
        ComputeShaderImpl& operator=(const ComputeShaderImpl&) = delete;
        ~ComputeShaderImpl();
    };
} // namespace p5
