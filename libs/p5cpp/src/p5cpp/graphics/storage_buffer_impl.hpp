#pragma once

#include <p5cpp/p5cpp.hpp>

#include <webgpu/webgpu.h>

namespace p5
{
    struct StorageBufferImpl
    {
        WGPUBuffer buffer = nullptr;

        StorageBufferImpl() = default;
        StorageBufferImpl(const StorageBufferImpl&) = delete;
        StorageBufferImpl& operator=(const StorageBufferImpl&) = delete;
        ~StorageBufferImpl();
    };
} // namespace p5
