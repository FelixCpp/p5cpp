#pragma once

#include <p5cpp/p5cpp.hpp>

#include "vertex.hpp"

namespace p5cpp
{
    struct DrawScope
    {
        size_t baseIndex;
        size_t baseVertex;
        size_t& indexCursor;
        size_t& vertexCursor;

        std::span<Vertex> vertices;
        std::span<uint32_t> indices;

        void pushVertex(const float2& position, const float2& texcoord, const float4& color);
        void pushTriangle(uint32_t a, uint32_t b, uint32_t c);
    };
} // namespace p5cpp

namespace p5cpp
{
    struct UniformCache;

    struct Renderer
    {
        static std::unique_ptr<Renderer> create(size_t vertexCount, size_t indexCount);

        virtual ~Renderer() = default;

        virtual void begin(Framebuffer* framebuffer) = 0;
        virtual void end() = 0;
        virtual void flush() = 0;

        virtual void submit(DrawScope scope, UniformCache& uniformCache, Shader* shader, BlendMode blendMode, Texture* texture) = 0;

        virtual DrawScope getDrawScope() = 0;
    };
} // namespace p5cpp
