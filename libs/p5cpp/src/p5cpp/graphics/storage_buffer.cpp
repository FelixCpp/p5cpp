#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/storage_buffer_impl.hpp>
#include <p5cpp/graphics/gpu_device.hpp>
#include <p5cpp/graphics/gpu_command.hpp>
#include <p5cpp/graphics/gpu_readback.hpp>

#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

namespace p5
{
    StorageBufferImpl::~StorageBufferImpl()
    {
        if (buffer != nullptr) {
            wgpuBufferDestroy(buffer);
            wgpuBufferRelease(buffer);
        }
    }

    bool StorageBuffer::isValid() const
    {
        return impl != nullptr;
    }

    std::optional<StorageBuffer> createStorageBuffer(uint64_t byteSize, std::span<const uint8_t> initialData)
    {
        if (byteSize == 0) {
            error("createStorageBuffer() requires byteSize > 0");
            return std::nullopt;
        }
        if (not initialData.empty() and initialData.size() != byteSize) {
            error("createStorageBuffer() initialData size does not match byteSize");
            return std::nullopt;
        }

        GpuDevice& gpuDevice = requireDependency<GpuDevice>();

        WGPUBufferDescriptor desc {};
        desc.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst;
        desc.size = byteSize;
        WGPUBuffer buffer = wgpuDeviceCreateBuffer(gpuDevice.getDevice(), &desc);
        if (buffer == nullptr) {
            error("createStorageBuffer() failed to allocate the GPU buffer");
            return std::nullopt;
        }

        if (not initialData.empty()) {
            wgpuQueueWriteBuffer(gpuDevice.getQueue(), buffer, 0, initialData.data(), initialData.size());
        }

        auto impl = std::make_shared<StorageBufferImpl>();
        impl->buffer = buffer;

        return StorageBuffer {.impl = std::move(impl), .byteSize = byteSize};
    }

    void StorageBuffer::updateData(std::span<const uint8_t> data, uint64_t offset)
    {
        if (not isValid()) {
            error("StorageBuffer::updateData() called on an invalid StorageBuffer");
            return;
        }
        if (offset + data.size() > byteSize) {
            error("StorageBuffer::updateData() write would exceed the buffer's size");
            return;
        }

        GpuDevice& gpuDevice = requireDependency<GpuDevice>();
        wgpuQueueWriteBuffer(gpuDevice.getQueue(), impl->buffer, offset, data.data(), data.size());
    }

    std::vector<uint8_t> StorageBuffer::readData() const
    {
        if (not isValid()) {
            error("StorageBuffer::readData() called on an invalid StorageBuffer");
            return {};
        }

        GpuDevice& gpuDevice = requireDependency<GpuDevice>();
        WGPUDevice device = gpuDevice.getDevice();

        WGPUBuffer stagingBuffer = GpuStagingReadback::createStagingBuffer(device, byteSize);
        if (stagingBuffer == nullptr) {
            error("StorageBuffer::readData() failed to allocate a staging buffer");
            return {};
        }

        {
            GpuCommandScope commands(device);
            wgpuCommandEncoderCopyBufferToBuffer(commands.encoder(), impl->buffer, 0, stagingBuffer, 0, byteSize);
            commands.submit(gpuDevice.getQueue());
        }

        const GpuReadbackLayout layout {.mappedByteSize = byteSize};
        std::optional<std::vector<uint8_t>> result = GpuStagingReadback::mapAndCopyBlocking(device, stagingBuffer, layout);

        wgpuBufferDestroy(stagingBuffer);
        wgpuBufferRelease(stagingBuffer);

        return result.value_or(std::vector<uint8_t> {});
    }

    StorageBufferReader::~StorageBufferReader()
    {
        for (StorageBufferReaderSlot& slot : ring) {
            if (slot.buffer != nullptr) {
                wgpuBufferDestroy(static_cast<WGPUBuffer>(slot.buffer));
                wgpuBufferRelease(static_cast<WGPUBuffer>(slot.buffer));
            }
        }
    }

    std::unique_ptr<StorageBufferReader> createStorageBufferReader(uint64_t byteSize, uint32_t ringSize)
    {
        if (byteSize == 0) {
            error("createStorageBufferReader() requires byteSize > 0");
            return nullptr;
        }
        if (ringSize < 2) {
            error("createStorageBufferReader() requires a ringSize of at least 2, got {}", ringSize);
            return nullptr;
        }

        GpuDevice& gpuDevice = requireDependency<GpuDevice>();

        auto reader = std::make_unique<StorageBufferReader>();
        reader->byteSize = byteSize;
        reader->ring.resize(ringSize);

        for (StorageBufferReaderSlot& slot : reader->ring) {
            slot.buffer = GpuStagingReadback::createStagingBuffer(gpuDevice.getDevice(), byteSize);
        }

        return reader;
    }

    bool requestStorageBufferReadback(StorageBufferReader& reader, const StorageBuffer& buffer)
    {
        if (not buffer.isValid()) {
            error("requestStorageBufferReadback() called with an invalid StorageBuffer");
            return false;
        }
        if (buffer.byteSize != reader.byteSize) {
            error("requestStorageBufferReadback() buffer size ({}) does not match reader size ({})", buffer.byteSize, reader.byteSize);
            return false;
        }

        StorageBufferReaderSlot& slot = reader.ring[reader.writeIndex];
        WGPUBuffer stagingBuffer = static_cast<WGPUBuffer>(slot.buffer);

        if (slot.pending and slot.mapRequested and not slot.mapComplete) {
            warn("requestStorageBufferReadback(): the previous readback for this ring slot hasn't completed yet -- dropping this request (pollStorageBufferReadback() isn't keeping up)");
            return false;
        }

        const bool droppedUndrained = slot.pending;
        if (droppedUndrained) {
            wgpuBufferUnmap(stagingBuffer);
            warn("requestStorageBufferReadback(): dropping an undrained frame -- pollStorageBufferReadback() isn't keeping up");
        }

        GpuDevice& gpuDevice = requireDependency<GpuDevice>();
        {
            GpuCommandScope commands(gpuDevice.getDevice());
            wgpuCommandEncoderCopyBufferToBuffer(commands.encoder(), buffer.impl->buffer, 0, stagingBuffer, 0, reader.byteSize);
            commands.submit(gpuDevice.getQueue());
        }

        slot.mapRequested = true;
        slot.pending = true;
        GpuStagingReadback::beginMapAsync(stagingBuffer, reader.byteSize, slot.mapComplete);

        reader.writeIndex = (reader.writeIndex + 1) % reader.ring.size();
        return not droppedUndrained;
    }

    std::optional<std::vector<uint8_t>> pollStorageBufferReadback(StorageBufferReader& reader)
    {
        StorageBufferReaderSlot& slot = reader.ring[reader.readIndex];
        if (not slot.pending) {
            return std::nullopt;
        }

        GpuDevice& gpuDevice = requireDependency<GpuDevice>();
        wgpuDevicePoll(gpuDevice.getDevice(), /* wait */ false, nullptr);

        if (not slot.mapComplete) {
            return std::nullopt;
        }

        slot.pending = false;
        slot.mapRequested = false;
        slot.mapComplete = false;
        reader.readIndex = (reader.readIndex + 1) % reader.ring.size();

        WGPUBuffer stagingBuffer = static_cast<WGPUBuffer>(slot.buffer);
        const GpuReadbackLayout layout {.mappedByteSize = reader.byteSize};
        return GpuStagingReadback::finishMappedRead(stagingBuffer, layout);
    }
} // namespace p5
