#include <p5cpp/graphics/gpu_command.hpp>

namespace p5
{
    GpuCommandScope::GpuCommandScope(WGPUDevice device)
        : m_encoder(wgpuDeviceCreateCommandEncoder(device, nullptr))
    {
    }

    GpuCommandScope::~GpuCommandScope()
    {
        if (not m_submitted) {
            wgpuCommandEncoderRelease(m_encoder);
        }
    }

    void GpuCommandScope::submit(WGPUQueue queue)
    {
        if (m_submitted) {
            return;
        }

        WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(m_encoder, nullptr);
        wgpuQueueSubmit(queue, 1, &commandBuffer);
        wgpuCommandBufferRelease(commandBuffer);
        wgpuCommandEncoderRelease(m_encoder);
        m_submitted = true;
    }
} // namespace p5
