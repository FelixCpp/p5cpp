#include "p5.hpp"
#include "geometry.hpp"

#include <optional>
#include <functional>
#include <glad/glad.h>

namespace p5
{
    struct DrawSubmission
    {
        std::optional<GLuint> shaderId;
        std::optional<GLuint> textureId;
        BlendMode blendMode;
    };

    struct WritableBuffer
    {
        std::span<Vertex> vertices;
        std::span<uint32_t> indices;
    };

    struct BufferWriteResult
    {
        size_t indexStart;

        size_t vertexCount;
        size_t indexCount;
    };

    struct GeometryRenderer
    {
        WritableBuffer buffer;

        BufferWriteResult convex(const std::span<const GeometryPoint>& points);
        BufferWriteResult concave(const std::span<const GeometryPoint>& points);
        BufferWriteResult stroke(const std::span<const GeometryPoint>& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, StrokeAlign strokeAlign, bool closed);
    };

    struct Renderer
    {
        virtual ~Renderer() = default;
        virtual void beginDraw() = 0;
        virtual void endDraw() = 0;
        virtual void draw(const DrawSubmission& submission, const std::function<BufferWriteResult(GeometryRenderer&)>& func) = 0;
    };

    std::unique_ptr<Renderer> createRenderer();

} // namespace p5
