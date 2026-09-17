#pragma once

#include <webgpu/webgpu.h>

namespace p5
{
    class GpuCommandScope
    {
    public:
        explicit GpuCommandScope(WGPUDevice device);
        ~GpuCommandScope();

        GpuCommandScope(const GpuCommandScope&) = delete;
        GpuCommandScope& operator=(const GpuCommandScope&) = delete;

        WGPUCommandEncoder encoder() const { return m_encoder; }
        void submit(WGPUQueue queue);

    private:
        WGPUCommandEncoder m_encoder;
        bool m_submitted = false;
    };
} // namespace p5
