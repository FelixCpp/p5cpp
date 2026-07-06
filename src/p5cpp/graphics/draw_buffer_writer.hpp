#pragma once

#include <p5cpp/math/value2.hpp>
#include <p5cpp/math/value4.hpp>

namespace p5cpp
{
    struct DrawBufferWriter
    {
        virtual ~DrawBufferWriter() = default;
        virtual uint32_t getRelativeCursor() const = 0;
        virtual void pushVertex(const float2& position, const float2& texcoord, const float4& color) = 0;
        virtual void pushTriangle(uint32_t a, uint32_t b, uint32_t c) = 0;
    };
} // namespace p5cpp
