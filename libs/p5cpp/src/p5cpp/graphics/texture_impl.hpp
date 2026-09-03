#pragma once

#include <p5cpp/p5cpp.hpp>

namespace p5
{
    struct TextureImpl
    {
        uint32_t id = 0;

        TextureImpl() = default;
        TextureImpl(const TextureImpl&) = delete;
        TextureImpl& operator=(const TextureImpl&) = delete;
        ~TextureImpl();
    };
} // namespace p5
