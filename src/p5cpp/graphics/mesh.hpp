#pragma once

#include <p5cpp/p5cpp.hpp>

namespace p5
{
    struct Mesh
    {
        virtual ~Mesh() = default;
        virtual void add(const float2& position, const float2& texCoord, const float4& color) = 0;
    };
} // namespace p5
