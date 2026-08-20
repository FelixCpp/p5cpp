#pragma once

#include <p5cpp/p5cpp.hpp>

namespace p5
{
    class VertexSink
    {
    public:
        virtual ~VertexSink() = default;
        virtual void addVertex(const float2& position, const float2& texCoord, const float4& color) = 0;
        virtual void addIndex(uint32_t index) = 0;
    };
} // namespace p5
