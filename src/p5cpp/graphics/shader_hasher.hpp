#pragma once

#include <p5cpp/graphics/shader.hpp>

#include <cstddef>
#include <functional>

namespace p5cpp
{
    struct ShaderHasher
    {
        constexpr size_t operator()(const ShaderId& shaderId) const noexcept
        {
            return std::hash<uint32_t>()(shaderId.value);
        }
    };
} // namespace p5cpp
