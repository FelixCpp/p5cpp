#include <p5cpp/graphics/extra_uniforms_ring.hpp>
#include <p5cpp/graphics/renderer.hpp>
#include <p5cpp/graphics/shader_impl.hpp>
#include <p5cpp/graphics/gpu_device.hpp>

#include <algorithm>
#include <cstring>
#include <type_traits>
#include <vector>

namespace p5
{
    namespace
    {
        matrix4x4 transposed(const matrix4x4& m)
        {
            matrix4x4 result;
            for (int row = 0; row < 4; ++row) {
                for (int col = 0; col < 4; ++col) {
                    result.m[col * 4 + row] = m.m[row * 4 + col];
                }
            }
            return result;
        }
    } // namespace

    ExtraUniformsRing::ExtraUniformsRing(GpuDevice& gpuDevice, WGPUBindGroupLayout layout, uint32_t slotStride, uint32_t capacity)
        : m_gpuDevice(gpuDevice),
          m_slotStride(slotStride),
          m_capacity(capacity),
          m_writeCursor(0)
    {
        WGPUBufferDescriptor bufferDesc {};
        bufferDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        bufferDesc.size = capacity;
        m_buffer = wgpuDeviceCreateBuffer(gpuDevice.getDevice(), &bufferDesc);

        WGPUBindGroupEntry entry {};
        entry.binding = 0;
        entry.buffer = m_buffer;
        entry.size = slotStride;

        WGPUBindGroupDescriptor bindDesc {};
        bindDesc.layout = layout;
        bindDesc.entryCount = 1;
        bindDesc.entries = &entry;
        m_bindGroup = wgpuDeviceCreateBindGroup(gpuDevice.getDevice(), &bindDesc);
    }

    ExtraUniformsRing::~ExtraUniformsRing()
    {
        wgpuBindGroupRelease(m_bindGroup);
        wgpuBufferDestroy(m_buffer);
        wgpuBufferRelease(m_buffer);
    }

    void ExtraUniformsRing::resetForFrame()
    {
        m_writeCursor = 0;
    }

    uint32_t ExtraUniformsRing::write(const RendererBatch& batch)
    {
        if (batch.shader.impl->extraUniformSlots.empty()) {
            return 0;
        }

        if (m_writeCursor + m_slotStride > m_capacity) {
            error("Renderer: extra-uniforms buffer exhausted mid-flush() (an unusually large number of distinct shader/texture batches in one flush) -- reusing slot 0, which may corrupt an earlier batch's uniforms this frame");
            m_writeCursor = 0;
        }

        std::vector<uint8_t> scratch(batch.shader.impl->extraUniformsByteSize, 0);
        for (const auto& [name, slot] : batch.shader.impl->extraUniformSlots) {
            const auto it = std::find_if(batch.uniforms.begin(), batch.uniforms.end(), [&](const auto& pair) { return pair.first == name; });
            if (it == batch.uniforms.end()) {
                continue;
            }

            std::visit([&](const auto& value) {
                using T = std::decay_t<decltype(value)>;
                if (sizeof(T) != slot.byteSize) {
                    warn("setUniform(\"{}\", ...) value type doesn't match the shader's declared @group(2) field type -- ignored", name);
                    return;
                }
                if constexpr (std::is_same_v<T, matrix4x4>) {
                    const matrix4x4 wgslValue = transposed(value);
                    std::memcpy(scratch.data() + slot.byteOffset, wgslValue.m.data(), sizeof(T));
                } else {
                    std::memcpy(scratch.data() + slot.byteOffset, &value, sizeof(T));
                }
            },
                       it->second);
        }

        const uint32_t offset = m_writeCursor;
        wgpuQueueWriteBuffer(m_gpuDevice.getQueue(), m_buffer, offset, scratch.data(), scratch.size());

        m_writeCursor += m_slotStride;
        return offset;
    }
} // namespace p5
