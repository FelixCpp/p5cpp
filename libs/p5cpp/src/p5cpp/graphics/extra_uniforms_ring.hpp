#pragma once

#include <p5cpp/p5cpp.hpp>

#include <webgpu/webgpu.h>

namespace p5
{
    class GpuDevice;
    struct RendererBatch;

    class ExtraUniformsRing
    {
    public:
        ExtraUniformsRing(GpuDevice& gpuDevice, WGPUBindGroupLayout layout, uint32_t slotStride, uint32_t capacity);
        ~ExtraUniformsRing();

        ExtraUniformsRing(const ExtraUniformsRing&) = delete;
        ExtraUniformsRing& operator=(const ExtraUniformsRing&) = delete;

        void resetForFrame();
        uint32_t write(const RendererBatch& batch);

        WGPUBindGroup bindGroup() const { return m_bindGroup; }

    private:
        GpuDevice& m_gpuDevice;
        WGPUBuffer m_buffer;
        WGPUBindGroup m_bindGroup;
        uint32_t m_slotStride;
        uint32_t m_capacity;
        uint32_t m_writeCursor;
    };
} // namespace p5
