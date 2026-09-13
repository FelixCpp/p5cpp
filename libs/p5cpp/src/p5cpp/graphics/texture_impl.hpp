#pragma once

#include <p5cpp/p5cpp.hpp>

#include <webgpu/webgpu.h>

#include <optional>

namespace p5
{
    struct TextureImpl
    {
        WGPUTexture texture = nullptr;
        WGPUTextureView view = nullptr;

        TextureImpl() = default;
        TextureImpl(const TextureImpl&) = delete;
        TextureImpl& operator=(const TextureImpl&) = delete;
        ~TextureImpl();
    };

    std::optional<WGPUTextureFormat> toWGPUTextureFormat(TexturePixelFormat format);
} // namespace p5
