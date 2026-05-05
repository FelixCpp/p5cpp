#pragma once

#include "p5.hpp"
#include "drawscope.hpp"

#include <span>
#include <optional>
#include <functional>
#include <memory>

namespace p5
{
    enum class DrawMode {
        triangles,
        lineLoop,
    };

    struct DrawSettings
    {
        uint32_t shaderId;
        std::optional<uint32_t> textureId;
        BlendMode blendMode;
        DrawMode drawMode;
    };

    struct DrawPoints
    {
        size_t size;
        std::span<const float2> positions;
        std::span<const float2> texcoords;
        std::span<const color_t> colors;
    };

    struct Camera
    {
        matrix4x4 projection;
    };

    struct Renderer
    {
        virtual ~Renderer() = default;
        virtual void beginDraw(const Camera& camera) = 0;
        virtual void endDraw() = 0;
        virtual void draw(const DrawSettings& settings, const std::function<void(DrawScope&)>& callback) = 0;
    };

    std::unique_ptr<Renderer> createRenderer();
} // namespace p5
