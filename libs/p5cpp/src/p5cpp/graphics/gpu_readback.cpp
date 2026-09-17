#include <p5cpp/graphics/gpu_readback.hpp>
#include <p5cpp/p5cpp.hpp>

#include <webgpu/wgpu.h>

#include <cstring>

namespace p5
{
    namespace
    {
        void beginBufferMapRead(WGPUBuffer buffer, uint64_t size, bool* complete)
        {
            WGPUBufferMapCallbackInfo callbackInfo {};
            callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
            callbackInfo.callback = [](WGPUMapAsyncStatus status, WGPUStringView message, void* userdata1, void*) {
                if (status != WGPUMapAsyncStatus_Success) {
                    error("GPU buffer map failed: {}", std::string_view(message.data, message.length));
                }
                *static_cast<bool*>(userdata1) = true;
            };
            callbackInfo.userdata1 = complete;

            wgpuBufferMapAsync(buffer, WGPUMapMode_Read, 0, size, callbackInfo);
        }

        std::optional<std::vector<uint8_t>> copyMappedBytes(WGPUBuffer buffer, const GpuReadbackLayout& layout)
        {
            const void* mapped = wgpuBufferGetConstMappedRange(buffer, 0, layout.mappedByteSize);
            if (mapped == nullptr) {
                error("GPU readback failed to read its mapped buffer");
                wgpuBufferUnmap(buffer);
                return std::nullopt;
            }

            const uint8_t* mappedBytes = static_cast<const uint8_t*>(mapped);
            std::vector<uint8_t> result;
            if (layout.rowStrideBytes == 0) {
                result.assign(mappedBytes, mappedBytes + layout.mappedByteSize);
            } else {
                result.resize(static_cast<size_t>(layout.rowSizeBytes) * layout.rowCount);
                for (uint32_t row = 0; row < layout.rowCount; ++row) {
                    std::memcpy(
                        result.data() + static_cast<size_t>(row) * layout.rowSizeBytes,
                        mappedBytes + static_cast<size_t>(row) * layout.rowStrideBytes,
                        layout.rowSizeBytes
                    );
                }
            }

            wgpuBufferUnmap(buffer);
            return result;
        }
    } // namespace

    WGPUBuffer GpuStagingReadback::createStagingBuffer(WGPUDevice device, uint64_t byteSize)
    {
        WGPUBufferDescriptor desc {};
        desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
        desc.size = byteSize;
        return wgpuDeviceCreateBuffer(device, &desc);
    }

    std::optional<std::vector<uint8_t>> GpuStagingReadback::mapAndCopyBlocking(WGPUDevice device, WGPUBuffer buffer, const GpuReadbackLayout& layout)
    {
        bool complete = false;
        beginBufferMapRead(buffer, layout.mappedByteSize, &complete);
        while (not complete) {
            wgpuDevicePoll(device, /* wait */ true, nullptr);
        }

        return copyMappedBytes(buffer, layout);
    }

    void GpuStagingReadback::beginMapAsync(WGPUBuffer buffer, uint64_t byteSize, bool& outComplete)
    {
        outComplete = false;
        beginBufferMapRead(buffer, byteSize, &outComplete);
    }

    std::optional<std::vector<uint8_t>> GpuStagingReadback::finishMappedRead(WGPUBuffer buffer, const GpuReadbackLayout& layout)
    {
        return copyMappedBytes(buffer, layout);
    }
} // namespace p5
