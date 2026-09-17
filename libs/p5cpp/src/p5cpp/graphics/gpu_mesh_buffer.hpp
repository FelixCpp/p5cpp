#pragma once

#include <p5cpp/graphics/gpu_device.hpp>

#include <webgpu/webgpu.h>

#include <vector>

namespace p5
{
    template <typename T>
    class GpuMeshBuffer
    {
    public:
        GpuMeshBuffer(GpuDevice& gpuDevice, WGPUBufferUsage usage, size_t initialCapacity)
            : m_gpuDevice(gpuDevice),
              m_usage(usage),
              m_data(initialCapacity),
              m_uploadedCapacity(initialCapacity)
        {
            WGPUBufferDescriptor desc {};
            desc.usage = m_usage;
            desc.size = initialCapacity * sizeof(T);
            m_buffer = wgpuDeviceCreateBuffer(m_gpuDevice.getDevice(), &desc);
        }

        ~GpuMeshBuffer()
        {
            wgpuBufferDestroy(m_buffer);
            wgpuBufferRelease(m_buffer);
        }

        GpuMeshBuffer(const GpuMeshBuffer&) = delete;
        GpuMeshBuffer& operator=(const GpuMeshBuffer&) = delete;

        void ensureCapacityFor(size_t index)
        {
            if (index >= m_data.size()) {
                m_data.resize(m_data.size() * 2);
            }

            if (m_data.size() > m_uploadedCapacity) {
                wgpuBufferDestroy(m_buffer);
                wgpuBufferRelease(m_buffer);

                WGPUBufferDescriptor desc {};
                desc.usage = m_usage;
                desc.size = m_data.size() * sizeof(T);
                m_buffer = wgpuDeviceCreateBuffer(m_gpuDevice.getDevice(), &desc);

                m_uploadedCapacity = m_data.size();
            }
        }

        void upload(WGPUQueue queue, size_t count) const
        {
            wgpuQueueWriteBuffer(queue, m_buffer, 0, m_data.data(), count * sizeof(T));
        }

        WGPUBuffer buffer() const { return m_buffer; }
        std::vector<T>& data() { return m_data; }

    private:
        GpuDevice& m_gpuDevice;
        WGPUBufferUsage m_usage;
        WGPUBuffer m_buffer;
        std::vector<T> m_data;
        size_t m_uploadedCapacity;
    };
} // namespace p5
