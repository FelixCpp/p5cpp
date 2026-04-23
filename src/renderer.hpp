#pragma once

#include "p5.hpp"

#include <span>
#include <optional>
#include <functional>
#include <memory>

namespace p5
{
    struct Vertex
    {
        float3 position;
        float2 texcoord;
        float4 color;
    };

    enum class DrawMode {
        triangles,
        lineLoop,
    };

    struct DrawSettings
    {
        std::optional<uint32_t> shaderId;
        std::optional<uint32_t> textureId;
        BlendMode blendMode;
        DrawMode drawMode;
    };

    struct DrawPoint
    {
        float2 position;
        float2 texcoord;
        color_t fillColor;
        color_t strokeColor;
    };

    struct DrawScope
    {
        std::span<Vertex> vertices;
        std::span<uint32_t> indices;

        size_t vertexStart;
        size_t vertexCount;

        size_t indexStart;
        size_t indexCount;
    };

    void convex(DrawScope& scope, const std::span<const DrawPoint>& points);
    void concave(DrawScope& scope, const std::span<const DrawPoint>& points);

    struct Renderer
    {
        virtual ~Renderer() = default;
        virtual void beginDraw() = 0;
        virtual void endDraw() = 0;
        virtual void draw(const DrawSettings& settings, const std::function<void(DrawScope&)>& callback) = 0;
    };

    std::unique_ptr<Renderer> createRenderer();
} // namespace p5
