#pragma once

#include <p5cpp/p5cpp.hpp>

#include <webgpu/webgpu.h>

namespace p5
{
    struct GraphicsImpl
    {
        WGPUTexture msaaTexture = nullptr;
        WGPUTextureView msaaView = nullptr;

        GraphicsImpl() = default;
        GraphicsImpl(const GraphicsImpl&) = delete;
        GraphicsImpl& operator=(const GraphicsImpl&) = delete;
        ~GraphicsImpl();
    };
} // namespace p5
