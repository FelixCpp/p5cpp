#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/texture_impl.hpp>
#include <p5cpp/graphics/gpu_device.hpp>
#include <p5cpp/graphics/gpu_command.hpp>
#include <p5cpp/graphics/gpu_readback.hpp>

#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include <cstring>
#include <optional>

namespace p5
{
    namespace
    {
        struct GpuPixelFormat
        {
            WGPUTextureFormat format;
            uint32_t bytesPerPixel;
        };

        std::optional<GpuPixelFormat> toGpuPixelFormat(TexturePixelFormat format)
        {
            switch (format) {
                case TexturePixelFormat::rgba8: return GpuPixelFormat {WGPUTextureFormat_RGBA8Unorm, 4};
                case TexturePixelFormat::r8: return GpuPixelFormat {WGPUTextureFormat_R8Unorm, 1};
                default:
                    error("Texture: unknown TexturePixelFormat");
                    return std::nullopt;
            }
        }

        constexpr uint32_t kCopyBytesPerRowAlignment = 256;

        uint32_t alignedBytesPerRow(uint32_t width, uint32_t bytesPerPixel)
        {
            const uint32_t unaligned = width * bytesPerPixel;
            return (unaligned + kCopyBytesPerRowAlignment - 1) / kCopyBytesPerRowAlignment * kCopyBytesPerRowAlignment;
        }

        std::optional<std::vector<uint8_t>> queryPixelData(const Texture& texture)
        {
            GpuDevice& gpuDevice = requireDependency<GpuDevice>();
            WGPUDevice device = gpuDevice.getDevice();

            const uint32_t bytesPerRow = alignedBytesPerRow(texture.size.x, 4);
            const uint64_t bufferSize = static_cast<uint64_t>(bytesPerRow) * texture.size.y;

            WGPUBuffer stagingBuffer = GpuStagingReadback::createStagingBuffer(device, bufferSize);
            if (stagingBuffer == nullptr) {
                error("Texture readback failed to allocate a staging buffer");
                return std::nullopt;
            }

            {
                GpuCommandScope commands(device);

                WGPUTexelCopyTextureInfo src {};
                src.texture = texture.impl->texture;
                src.origin = WGPUOrigin3D {0, 0, 0};
                src.aspect = WGPUTextureAspect_All;

                WGPUTexelCopyBufferInfo dst {};
                dst.buffer = stagingBuffer;
                dst.layout.bytesPerRow = bytesPerRow;
                dst.layout.rowsPerImage = texture.size.y;

                const WGPUExtent3D copySize {texture.size.x, texture.size.y, 1};
                wgpuCommandEncoderCopyTextureToBuffer(commands.encoder(), &src, &dst, &copySize);
                commands.submit(gpuDevice.getQueue());
            }

            const GpuReadbackLayout layout {
                .mappedByteSize = bufferSize,
                .rowStrideBytes = bytesPerRow,
                .rowSizeBytes = texture.size.x * 4,
                .rowCount = texture.size.y,
            };
            std::optional<std::vector<uint8_t>> result = GpuStagingReadback::mapAndCopyBlocking(device, stagingBuffer, layout);

            wgpuBufferDestroy(stagingBuffer);
            wgpuBufferRelease(stagingBuffer);

            return result;
        }
    } // namespace

    std::optional<WGPUTextureFormat> toWGPUTextureFormat(TexturePixelFormat format)
    {
        const std::optional<GpuPixelFormat> gpuFormat = toGpuPixelFormat(format);
        if (not gpuFormat.has_value()) {
            return std::nullopt;
        }
        return gpuFormat->format;
    }

    TextureImpl::~TextureImpl()
    {
        if (view != nullptr) {
            wgpuTextureViewRelease(view);
        }
        if (texture != nullptr) {
            wgpuTextureDestroy(texture);
            wgpuTextureRelease(texture);
        }
    }

    std::optional<Texture> loadTexture(uint32_t width, uint32_t height, std::span<const uint8_t> data, TexturePixelFormat format)
    {
        const std::optional<GpuPixelFormat> gpuFormatOpt = toGpuPixelFormat(format);
        if (not gpuFormatOpt.has_value()) {
            return std::nullopt;
        }
        const GpuPixelFormat& gpuFormat = *gpuFormatOpt;

        const size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * gpuFormat.bytesPerPixel;
        if (not data.empty() and data.size() != expectedSize) {
            error("loadTexture() data size does not match width * height * bytesPerPixel");
            return std::nullopt;
        }

        GpuDevice& gpuDevice = requireDependency<GpuDevice>();

        WGPUTextureDescriptor desc {};
        desc.dimension = WGPUTextureDimension_2D;
        desc.size = WGPUExtent3D {width, height, 1};
        desc.format = gpuFormat.format;
        desc.mipLevelCount = 1;
        desc.sampleCount = 1;
        desc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst | WGPUTextureUsage_CopySrc | WGPUTextureUsage_RenderAttachment;

        WGPUTexture texture = wgpuDeviceCreateTexture(gpuDevice.getDevice(), &desc);
        if (texture == nullptr) {
            error("loadTexture() failed to create the GPU texture");
            return std::nullopt;
        }

        if (not data.empty()) {
            WGPUTexelCopyTextureInfo dst {};
            dst.texture = texture;
            dst.origin = WGPUOrigin3D {0, 0, 0};
            dst.aspect = WGPUTextureAspect_All;

            WGPUTexelCopyBufferLayout layout {};
            layout.bytesPerRow = width * gpuFormat.bytesPerPixel;
            layout.rowsPerImage = height;

            const WGPUExtent3D writeSize {width, height, 1};
            wgpuQueueWriteTexture(gpuDevice.getQueue(), &dst, data.data(), data.size(), &layout, &writeSize);
        }

        auto impl = std::make_shared<TextureImpl>();
        impl->texture = texture;
        impl->view = wgpuTextureCreateView(texture, nullptr);

        return Texture {.impl = std::move(impl), .size = uint2 {.x = width, .y = height}, .pixelFormat = format};
    }

    std::optional<Texture> loadTexture(const std::filesystem::path& filepath)
    {
        typedef decltype(&stbi_image_free) stbi_deleter;

        const std::string filepathStr = filepath.string();
        int width, height, channels;
        std::unique_ptr<stbi_uc, stbi_deleter> pixelData(stbi_load(filepathStr.c_str(), &width, &height, &channels, STBI_rgb_alpha), &stbi_image_free);

        if (pixelData == nullptr) {
            return std::nullopt;
        }

        return loadTexture(static_cast<uint32_t>(width), static_cast<uint32_t>(height), std::span<const uint8_t>(pixelData.get(), width * height * 4), TexturePixelFormat::rgba8);
    }

    void Texture::updateSubImage(uint32_t x, uint32_t y, uint32_t width, uint32_t height, std::span<const uint8_t> data)
    {
        const std::optional<GpuPixelFormat> gpuFormatOpt = toGpuPixelFormat(pixelFormat);
        if (not gpuFormatOpt.has_value()) {
            return;
        }
        const GpuPixelFormat& gpuFormat = *gpuFormatOpt;

        const size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * gpuFormat.bytesPerPixel;
        if (data.size() != expectedSize) {
            error("updateSubImage() data size does not match width * height * bytesPerPixel");
            return;
        }
        if (x + width > size.x or y + height > size.y) {
            error("updateSubImage() region is out of bounds");
            return;
        }

        GpuDevice& gpuDevice = requireDependency<GpuDevice>();

        WGPUTexelCopyTextureInfo dst {};
        dst.texture = impl->texture;
        dst.origin = WGPUOrigin3D {x, y, 0};
        dst.aspect = WGPUTextureAspect_All;

        WGPUTexelCopyBufferLayout layout {};
        layout.bytesPerRow = width * gpuFormat.bytesPerPixel;
        layout.rowsPerImage = height;

        const WGPUExtent3D writeSize {width, height, 1};
        wgpuQueueWriteTexture(gpuDevice.getQueue(), &dst, data.data(), data.size(), &layout, &writeSize);
    }

    Texture Texture::getSubTexture(uint32_t x, uint32_t y, uint32_t width, uint32_t height) const
    {
        if (width == 0 or height == 0) {
            error("getSubTexture() region must have non-zero width and height");
            return {};
        }
        if (x + width > size.x or y + height > size.y) {
            error("getSubTexture() region is out of bounds");
            return {};
        }

        std::optional<Texture> subTexture = loadTexture(width, height, {}, pixelFormat);
        if (not subTexture.has_value()) {
            return {};
        }

        GpuDevice& gpuDevice = requireDependency<GpuDevice>();
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(gpuDevice.getDevice(), nullptr);

        WGPUTexelCopyTextureInfo src {};
        src.texture = impl->texture;
        src.origin = WGPUOrigin3D {x, y, 0};
        src.aspect = WGPUTextureAspect_All;

        WGPUTexelCopyTextureInfo dst {};
        dst.texture = subTexture->impl->texture;
        dst.origin = WGPUOrigin3D {0, 0, 0};
        dst.aspect = WGPUTextureAspect_All;

        const WGPUExtent3D copySize {width, height, 1};
        wgpuCommandEncoderCopyTextureToTexture(encoder, &src, &dst, &copySize);

        WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(gpuDevice.getQueue(), 1, &commandBuffer);
        wgpuCommandBufferRelease(commandBuffer);
        wgpuCommandEncoderRelease(encoder);

        return subTexture.value();
    }

    bool Texture::isValid() const
    {
        return impl != nullptr;
    }

    bool Texture::saveToFileAsPNG(const std::filesystem::path& filepath) const
    {
        const std::string filepathStr = filepath.string();
        const auto [width, height] = size;
        auto pixelData = queryPixelData(*this);
        if (not pixelData.has_value()) {
            return false;
        }
        const int result = stbi_write_png(filepathStr.c_str(), static_cast<int>(width), static_cast<int>(height), STBI_rgb_alpha, pixelData->data(), static_cast<int>(width) * 4);
        return result != 0;
    }

    bool Texture::saveToFileAsJPEG(const std::filesystem::path& filepath, int quality) const
    {
        const std::string filepathStr = filepath.string();
        const auto [width, height] = size;
        auto pixelData = queryPixelData(*this);
        if (not pixelData.has_value()) {
            return false;
        }
        const int result = stbi_write_jpg(filepathStr.c_str(), static_cast<int>(width), static_cast<int>(height), STBI_rgb_alpha, pixelData->data(), quality);
        return result != 0;
    }

    bool Texture::saveToFileAsBMP(const std::filesystem::path& filepath) const
    {
        const std::string filepathStr = filepath.string();
        const auto [width, height] = size;
        auto pixelData = queryPixelData(*this);
        if (not pixelData.has_value()) {
            return false;
        }
        const int result = stbi_write_bmp(filepathStr.c_str(), static_cast<int>(width), static_cast<int>(height), STBI_rgb_alpha, pixelData->data());
        return result != 0;
    }

    Pixels Texture::loadPixels() const
    {
        if (pixelFormat != TexturePixelFormat::rgba8) {
            error("loadPixels() only supports TexturePixelFormat::rgba8 textures");
            return {};
        }

        const auto [width, height] = size;
        auto bytes = queryPixelData(*this);
        if (not bytes.has_value()) {
            return {};
        }

        return Pixels {
            .width = width,
            .height = height,
            .data = std::move(bytes).value()
        };
    }

    void Texture::updatePixels(const Pixels& pixels)
    {
        if (pixelFormat != TexturePixelFormat::rgba8) {
            error("updatePixels() only supports TexturePixelFormat::rgba8 textures");
            return;
        }

        const auto [width, height] = size;
        if (pixels.width != width or pixels.height != height) {
            error("updatePixels() Pixels size ({}x{}) does not match texture size ({}x{})", pixels.width, pixels.height, width, height);
            return;
        }

        updateSubImage(0, 0, width, height, pixels.data);
    }

    PixelReader::~PixelReader()
    {
        for (PixelReaderSlot& slot : ring) {
            if (slot.buffer != nullptr) {
                wgpuBufferDestroy(static_cast<WGPUBuffer>(slot.buffer));
                wgpuBufferRelease(static_cast<WGPUBuffer>(slot.buffer));
            }
        }
    }

    std::unique_ptr<PixelReader> createPixelReader(uint32_t width, uint32_t height, uint32_t ringSize)
    {
        if (ringSize < 2) {
            error("createPixelReader() requires a ringSize of at least 2, got {}", ringSize);
            return nullptr;
        }

        GpuDevice& gpuDevice = requireDependency<GpuDevice>();

        auto reader = std::make_unique<PixelReader>();
        reader->width = width;
        reader->height = height;
        reader->ring.resize(ringSize);

        const uint64_t bufferSize = static_cast<uint64_t>(alignedBytesPerRow(width, 4)) * height;
        for (PixelReaderSlot& slot : reader->ring) {
            WGPUBufferDescriptor desc {};
            desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
            desc.size = bufferSize;
            slot.buffer = wgpuDeviceCreateBuffer(gpuDevice.getDevice(), &desc);
        }

        return reader;
    }

    bool requestPixelReadback(PixelReader& reader, const Texture& texture)
    {
        if (texture.pixelFormat != TexturePixelFormat::rgba8) {
            error("requestPixelReadback() only supports TexturePixelFormat::rgba8 textures");
            return false;
        }
        if (texture.size.x != reader.width or texture.size.y != reader.height) {
            error("requestPixelReadback() texture size ({}x{}) does not match reader size ({}x{})", texture.size.x, texture.size.y, reader.width, reader.height);
            return false;
        }

        PixelReaderSlot& slot = reader.ring[reader.writeIndex];
        WGPUBuffer buffer = static_cast<WGPUBuffer>(slot.buffer);

        if (slot.pending and slot.mapRequested and not slot.mapComplete) {
            warn("requestPixelReadback(): the previous readback for this ring slot hasn't completed yet -- dropping this request (pollPixelReadback() isn't keeping up)");
            return false;
        }

        const bool droppedUndrained = slot.pending;
        if (droppedUndrained) {
            wgpuBufferUnmap(buffer);
            warn("requestPixelReadback(): dropping an undrained frame -- pollPixelReadback() isn't keeping up");
        }

        GpuDevice& gpuDevice = requireDependency<GpuDevice>();
        {
            GpuCommandScope commands(gpuDevice.getDevice());

            WGPUTexelCopyTextureInfo src {};
            src.texture = texture.impl->texture;
            src.origin = WGPUOrigin3D {0, 0, 0};
            src.aspect = WGPUTextureAspect_All;

            WGPUTexelCopyBufferInfo dst {};
            dst.buffer = buffer;
            dst.layout.bytesPerRow = alignedBytesPerRow(reader.width, 4);
            dst.layout.rowsPerImage = reader.height;

            const WGPUExtent3D copySize {reader.width, reader.height, 1};
            wgpuCommandEncoderCopyTextureToBuffer(commands.encoder(), &src, &dst, &copySize);
            commands.submit(gpuDevice.getQueue());
        }

        slot.mapRequested = true;
        slot.pending = true;
        const uint64_t bufferSize = static_cast<uint64_t>(alignedBytesPerRow(reader.width, 4)) * reader.height;
        GpuStagingReadback::beginMapAsync(buffer, bufferSize, slot.mapComplete);

        reader.writeIndex = (reader.writeIndex + 1) % reader.ring.size();
        return not droppedUndrained;
    }

    std::optional<Pixels> pollPixelReadback(PixelReader& reader)
    {
        PixelReaderSlot& slot = reader.ring[reader.readIndex];
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

        WGPUBuffer buffer = static_cast<WGPUBuffer>(slot.buffer);
        const uint32_t bytesPerRow = alignedBytesPerRow(reader.width, 4);
        const uint64_t bufferSize = static_cast<uint64_t>(bytesPerRow) * reader.height;

        const GpuReadbackLayout layout {
            .mappedByteSize = bufferSize,
            .rowStrideBytes = bytesPerRow,
            .rowSizeBytes = reader.width * 4,
            .rowCount = reader.height,
        };
        std::optional<std::vector<uint8_t>> bytes = GpuStagingReadback::finishMappedRead(buffer, layout);
        if (not bytes.has_value()) {
            return std::nullopt;
        }

        return Pixels {.width = reader.width, .height = reader.height, .data = std::move(bytes).value()};
    }
} // namespace p5
