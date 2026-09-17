#pragma once

#include <webgpu/webgpu.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace p5
{
    struct GpuReadbackLayout
    {
        uint64_t mappedByteSize = 0;
        uint32_t rowStrideBytes = 0; // 0 => copy mappedByteSize bytes contiguously, no row splitting
        uint32_t rowSizeBytes = 0;   // used only when rowStrideBytes != 0
        uint32_t rowCount = 0;       // used only when rowStrideBytes != 0
    };

    class GpuStagingReadback
    {
    public:
        static WGPUBuffer createStagingBuffer(WGPUDevice device, uint64_t byteSize);

        static std::optional<std::vector<uint8_t>> mapAndCopyBlocking(WGPUDevice device, WGPUBuffer buffer, const GpuReadbackLayout& layout);
        static void beginMapAsync(WGPUBuffer buffer, uint64_t byteSize, bool& outComplete);
        static std::optional<std::vector<uint8_t>> finishMappedRead(WGPUBuffer buffer, const GpuReadbackLayout& layout);
    };
} // namespace p5
