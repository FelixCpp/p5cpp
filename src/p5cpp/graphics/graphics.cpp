#include <p5cpp/graphics/tessellators.hpp>
#include <p5cpp/graphics/graphics.hpp>

#include <cassert>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace p5
{
    namespace detail
    {
        inline static constexpr std::string_view defaultVertexShaderSource = R"(
            #version 410

            layout (location = 0) in vec2 a_Position;
            layout (location = 1) in vec2 a_TexCoord;
            layout (location = 2) in vec4 a_Color;

            uniform mat4 u_ProjectionMatrix;

            out vec2 v_TexCoord;
            out vec4 v_Color;

            void main()
            {
                gl_Position = u_ProjectionMatrix * vec4(a_Position, 0.0, 1.0);
                v_TexCoord = a_TexCoord;
                v_Color = a_Color;
            }
        )";

        inline static constexpr std::string_view defaultFragmentShaderSource = R"(
            #version 410

            layout (location = 0) out vec4 o_FragColor;

            in vec2 v_TexCoord;
            in vec4 v_Color;

            uniform sampler2D u_Texture;

            void main()
            {
                o_FragColor = texture(u_Texture, v_TexCoord) * v_Color;
            }
        )";

        inline static constexpr size_t MAX_VERTICES = 4096;
        inline static constexpr size_t MAX_INDICES = 6144;

        float4 toFloat4(color_t color)
        {
            return {
                static_cast<float>(getRed(color)) / 255.0f,
                static_cast<float>(getGreen(color)) / 255.0f,
                static_cast<float>(getBlue(color)) / 255.0f,
                static_cast<float>(getAlpha(color)) / 255.0f,
            };
        }

        int ellipseSegmentCount(float radiusX, float radiusY)
        {
            const float maxRadius = std::max(std::abs(radiusX), std::abs(radiusY));
            const int segments = static_cast<int>(std::ceil(std::numbers::pi_v<float> * std::sqrt(2.0f * maxRadius)));
            return std::clamp(segments, 16, 256);
        }

        void buildEllipsePoints(float centerX, float centerY, float radiusX, float radiusY, std::vector<float2>& positions, std::vector<float2>& texCoords)
        {
            const int segments = ellipseSegmentCount(radiusX, radiusY);
            positions.resize(segments);
            texCoords.resize(segments);

            for (int i = 0; i < segments; ++i) {
                const float angle = (2.0f * std::numbers::pi_v<float> * static_cast<float>(i)) / static_cast<float>(segments);
                const float c = std::cos(angle);
                const float s = std::sin(angle);
                positions[i] = {centerX + c * radiusX, centerY + s * radiusY};
                texCoords[i] = {0.5f + 0.5f * c, 0.5f + 0.5f * s};
            }
        }

        float distance2(const float2& a, const float2& b)
        {
            return std::hypot(b.x - a.x, b.y - a.y);
        }

        // Subdivision count for a cubic/quadratic Bezier or Catmull-Rom segment, sized to the length of
        // its control polygon (an upper bound on the curve's own arc length) rather than a fixed constant.
        int curveSegmentCount(float controlPolygonLength)
        {
            return std::clamp(static_cast<int>(std::ceil(controlPolygonLength / 3.0f)), 8, 128);
        }

        float2 cubicBezierPoint(const float2& p0, const float2& p1, const float2& p2, const float2& p3, float t)
        {
            const float u = 1.0f - t;
            const float b0 = u * u * u;
            const float b1 = 3.0f * u * u * t;
            const float b2 = 3.0f * u * t * t;
            const float b3 = t * t * t;
            return {
                b0 * p0.x + b1 * p1.x + b2 * p2.x + b3 * p3.x,
                b0 * p0.y + b1 * p1.y + b2 * p2.y + b3 * p3.y,
            };
        }

        float2 quadraticBezierPoint(const float2& p0, const float2& p1, const float2& p2, float t)
        {
            const float u = 1.0f - t;
            const float b0 = u * u;
            const float b1 = 2.0f * u * t;
            const float b2 = t * t;
            return {
                b0 * p0.x + b1 * p1.x + b2 * p2.x,
                b0 * p0.y + b1 * p1.y + b2 * p2.y,
            };
        }
    } // namespace detail
} // namespace p5

namespace p5
{
    Graphics::Graphics()
        : m_stateStack(),
          m_matrixStack(),
          m_renderer(Renderer::create(detail::MAX_VERTICES, detail::MAX_INDICES)),
          m_defaultFillShader(loadShaderFromMemory(detail::defaultVertexShaderSource, detail::defaultFragmentShaderSource)),
          m_defaultTexture(loadTextureFromMemory(1, 1, std::array<uint8_t, 4> {255, 255, 255, 255}))
    {
    }

    void Graphics::push()
    {
        pushState();
        pushMatrix();
    }

    void Graphics::pop()
    {
        popMatrix();
        popState();
    }

    void Graphics::pushFramebuffer(std::shared_ptr<Framebuffer> framebuffer)
    {
        m_renderer->end();
        m_renderer->begin(framebuffer);
        m_framebufferStack.push(std::move(framebuffer));
        push();
    }

    void Graphics::popFramebuffer()
    {
        pop();
        m_framebufferStack.pop();

        m_renderer->end();
        if (const auto newFramebuffer = m_framebufferStack.peek()) {
            m_renderer->begin(newFramebuffer);
        }
    }

    std::shared_ptr<Framebuffer> Graphics::peekFramebuffer() const
    {
        return m_framebufferStack.peek();
    }

    const uint2& Graphics::getFramebufferSize() const
    {
        return m_framebufferStack.peek()->getSize();
    }

    void Graphics::pushState()
    {
        m_stateStack.push(peekState());
    }

    void Graphics::popState()
    {
        m_stateStack.pop();
    }

    DrawState& Graphics::peekState()
    {
        return m_stateStack.peek();
    }

    void Graphics::pushMatrix()
    {
        m_matrixStack.push(peekMatrix());
    }

    void Graphics::popMatrix()
    {
        m_matrixStack.pop();
    }

    matrix4x4& Graphics::peekMatrix()
    {
        return m_matrixStack.peek();
    }

    void Graphics::applyMatrix(const matrix4x4& matrix)
    {
        m_matrixStack.set(m_matrixStack.peek() * matrix);
    }

    void Graphics::setMatrix(const matrix4x4& matrix)
    {
        m_matrixStack.set(matrix);
    }

    void Graphics::translate(float x, float y)
    {
        applyMatrix(translationMatrix(x, y));
    }

    void Graphics::scale(float x, float y)
    {
        applyMatrix(scalingMatrix(x, y));
    }

    void Graphics::rotate(float radians)
    {
        applyMatrix(rotationMatrix(radians));
    }

    void Graphics::fill(color_t color)
    {
        DrawState& state = peekState();
        state.isFillEnabled = true;
        state.fillColor = color;
    }

    void Graphics::noFill()
    {
        DrawState& state = peekState();
        state.isFillEnabled = false;
    }

    void Graphics::stroke(color_t color)
    {
        DrawState& state = peekState();
        state.isStrokeEnabled = true;
        state.strokeColor = color;
    }

    void Graphics::noStroke()
    {
        DrawState& state = peekState();
        state.isStrokeEnabled = false;
    }

    void Graphics::strokeWeight(float weight)
    {
        DrawState& state = peekState();
        state.strokeWeight = weight;
    }

    void Graphics::strokeCap(StrokeCap cap)
    {
        DrawState& state = peekState();
        state.strokeCap = cap;
    }

    void Graphics::strokeJoin(StrokeJoin join)
    {
        DrawState& state = peekState();
        state.strokeJoin = join;
    }

    void Graphics::strokeMiterLimit(float limit)
    {
        DrawState& state = peekState();
        state.strokeMiterLimit = limit;
    }

    void Graphics::strokeRoundJoinThreshold(float threshold)
    {
        DrawState& state = peekState();
        state.strokeRoundJoinThreshold = threshold;
    }

    void Graphics::curveTightness(float tightness)
    {
        DrawState& state = peekState();
        state.curveTightness = tightness;
    }

    void Graphics::tint(color_t color)
    {
        DrawState& state = peekState();
        state.tintColor = color;
    }

    void Graphics::noTint()
    {
        tint(rgba(255, 255, 255, 255));
    }

    void Graphics::blendMode(const BlendMode& blendMode)
    {
        DrawState& state = peekState();
        state.blendMode = blendMode;
    }

    void Graphics::shader(std::shared_ptr<Shader> shader)
    {
        DrawState& state = peekState();
        state.shader = shader;
    }

    void Graphics::noShader()
    {
        shader(nullptr);
    }

    void Graphics::setUniform(std::string_view name, float value)
    {
    }

    void Graphics::setUniform(std::string_view name, const float2& value)
    {
    }

    void Graphics::setUniform(std::string_view name, const float3& value)
    {
    }

    void Graphics::setUniform(std::string_view name, const float4& value)
    {
    }

    void Graphics::setUniform(std::string_view name, const matrix4x4& value)
    {
    }

    std::shared_ptr<Shader> Graphics::resolveActiveShader()
    {
        DrawState& state = peekState();
        if (state.shader != nullptr) {
            return state.shader;
        }

        return m_defaultFillShader;
    }

    std::shared_ptr<Texture> Graphics::resolveActiveTexture(const std::shared_ptr<Texture>& texture)
    {
        if (texture != nullptr) {
            return texture;
        }

        return m_defaultTexture;
    }

    void Graphics::submitQuad(const std::span<const float2, 4>& positions, const std::span<const float2, 4>& texCoords, color_t color, const DrawState& state, const std::shared_ptr<Texture>& texture)
    {
        const float4 col = detail::toFloat4(color);
        const float4 colors[4] = {col, col, col, col};

        Renderer::Writer writer = m_renderer->write();
        tesselate_quad(writer, positions, texCoords, colors);
        m_renderer->finish(writer, state.blendMode, resolveActiveTexture(texture), resolveActiveShader());
    }

    void Graphics::submitFan(const std::span<const float2>& positions, const std::span<const float2>& texCoords, color_t color, const DrawState& state)
    {
        if (positions.size() < 3)
            return;

        const float4 col = detail::toFloat4(color);
        const std::vector<float4> colors(positions.size(), col);

        Renderer::Writer writer = m_renderer->write();
        tesselate_triangle_fan(writer, positions, texCoords, colors);
        m_renderer->finish(writer, state.blendMode, resolveActiveTexture(), resolveActiveShader());
    }

    void Graphics::submitStroke(const std::span<const float2>& positions, bool closed, color_t color, const DrawState& state)
    {
        const std::vector<float2> texCoords(positions.size(), float2 {0.0f, 0.0f});
        const std::vector<color_t> colors(positions.size(), color);
        submitStroke(positions, texCoords, colors, closed, state);
    }

    void Graphics::submitStroke(const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const color_t>& colors, bool closed, const DrawState& state)
    {
        std::vector<float4> convertedColors(colors.size());
        std::ranges::transform(colors, convertedColors.begin(), detail::toFloat4);

        Renderer::Writer writer = m_renderer->write();
        tesselate_path(writer, positions, texCoords, convertedColors, state.strokeWeight, state.strokeCap, state.strokeJoin, state.strokeMiterLimit, state.strokeRoundJoinThreshold, closed);
        m_renderer->finish(writer, state.blendMode, resolveActiveTexture(), resolveActiveShader());
    }

    void Graphics::submitFillMesh(ShapeMode mode, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const color_t>& colors, const DrawState& state)
    {
        std::vector<float4> convertedColors(colors.size());
        std::ranges::transform(colors, convertedColors.begin(), detail::toFloat4);

        Renderer::Writer writer = m_renderer->write();
        switch (mode) {
            case ShapeMode::triangles: tesselate_triangles(writer, positions, texCoords, convertedColors); break;
            case ShapeMode::triangleStrip: tesselate_triangle_strip(writer, positions, texCoords, convertedColors); break;
            case ShapeMode::triangleFan: tesselate_triangle_fan(writer, positions, texCoords, convertedColors); break;
            case ShapeMode::quads: tesselate_quads(writer, positions, texCoords, convertedColors); break;
            case ShapeMode::quadStrip: tesselate_quad_strip(writer, positions, texCoords, convertedColors); break;
            case ShapeMode::points:
            case ShapeMode::lines: return; // points/lines have no interior to fill
            case ShapeMode::polygon:
            default: tesselate_polygon(writer, positions, texCoords, convertedColors); break;
        }
        m_renderer->finish(writer, state.blendMode, resolveActiveTexture(), resolveActiveShader());
    }

    void Graphics::background(color_t color)
    {
        const auto [width, height] = getFramebufferSize();
        const float2 positions[4] = {
            {0.0f, 0.0f},
            {static_cast<float>(width), 0.0f},
            {static_cast<float>(width), static_cast<float>(height)},
            {0.0f, static_cast<float>(height)},
        };

        const float2 texCoords[4] = {
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f},
        };

        submitQuad(positions, texCoords, color, peekState());
    }

    void Graphics::rect(float x, float y, float width, float height)
    {
        const float2 positions[4] = {
            {x, y},
            {x + width, y},
            {x + width, y + height},
            {x, y + height},
        };

        const float2 texCoords[4] = {
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f},
        };

        const DrawState& state = peekState();

        if (state.isFillEnabled) {
            submitQuad(positions, texCoords, state.fillColor, state);
        }

        if (state.isStrokeEnabled) {
            submitStroke(positions, true, state.strokeColor, state);
        }
    }

    void Graphics::square(float x, float y, float size)
    {
        rect(x, y, size, size);
    }

    void Graphics::ellipse(float x, float y, float radiusX, float radiusY)
    {
        std::vector<float2> positions;
        std::vector<float2> texCoords;
        detail::buildEllipsePoints(x, y, radiusX, radiusY, positions, texCoords);

        const DrawState& state = peekState();

        if (state.isFillEnabled) {
            submitFan(positions, texCoords, state.fillColor, state);
        }

        if (state.isStrokeEnabled) {
            submitStroke(positions, true, state.strokeColor, state);
        }
    }

    void Graphics::circle(float x, float y, float radius)
    {
        ellipse(x, y, radius, radius);
    }

    void Graphics::line(float x1, float y1, float x2, float y2)
    {
        const DrawState& state = peekState();
        if (not state.isStrokeEnabled)
            return;

        const float2 positions[2] = {{x1, y1}, {x2, y2}};
        submitStroke(positions, false, state.strokeColor, state);
    }

    void Graphics::triangle(float x1, float y1, float x2, float y2, float x3, float y3)
    {
        const float2 positions[3] = {{x1, y1}, {x2, y2}, {x3, y3}};
        const float2 texCoords[3] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {0.5f, 1.0f}};

        const DrawState& state = peekState();

        if (state.isFillEnabled) {
            const float4 fillColor = detail::toFloat4(state.fillColor);
            const float4 colors[3] = {fillColor, fillColor, fillColor};
            Renderer::Writer writer = m_renderer->write();
            tesselate_triangle(writer, positions, texCoords, colors);
            m_renderer->finish(writer, state.blendMode, resolveActiveTexture(), resolveActiveShader());
        }

        if (state.isStrokeEnabled) {
            submitStroke(positions, true, state.strokeColor, state);
        }
    }

    void Graphics::submitPoint(const float2& position, color_t color, const DrawState& state)
    {
        const float radius = std::max(state.strokeWeight, 1.0f) * 0.5f;
        const bool round = state.strokeCap.start == StrokeCapStyle::round or state.strokeCap.end == StrokeCapStyle::round;

        if (round) {
            std::vector<float2> positions;
            std::vector<float2> texCoords;
            detail::buildEllipsePoints(position.x, position.y, radius, radius, positions, texCoords);
            submitFan(positions, texCoords, color, state);
        } else {
            const float2 positions[4] = {
                {position.x - radius, position.y - radius},
                {position.x + radius, position.y - radius},
                {position.x + radius, position.y + radius},
                {position.x - radius, position.y + radius},
            };

            const float2 texCoords[4] = {
                {0.0f, 0.0f},
                {1.0f, 0.0f},
                {1.0f, 1.0f},
                {0.0f, 1.0f},
            };

            submitQuad(positions, texCoords, color, state);
        }
    }

    void Graphics::point(float x, float y)
    {
        const DrawState& state = peekState();
        if (not state.isStrokeEnabled)
            return;

        submitPoint({x, y}, state.strokeColor, state);
    }

    void Graphics::beginShape(ShapeMode mode)
    {
        m_shape = ShapeBuilder {};
        m_shape.active = true;
        m_shape.mode = mode;
    }

    void Graphics::appendShapeVertex(const float2& position, const float2& texCoord)
    {
        if (not m_shape.active)
            throw std::runtime_error("vertex() called without a matching beginShape()");

        const DrawState& state = peekState();
        m_shape.positions.push_back(position);
        m_shape.texCoords.push_back(texCoord);
        m_shape.fillColors.push_back(state.fillColor);
        m_shape.strokeColors.push_back(state.strokeColor);
    }

    void Graphics::vertex(float x, float y)
    {
        appendShapeVertex({x, y}, {0.0f, 0.0f});
    }

    void Graphics::vertex(float x, float y, float u, float v)
    {
        appendShapeVertex({x, y}, {u, v});
    }

    void Graphics::bezierVertex(float controlX1, float controlY1, float controlX2, float controlY2, float x, float y)
    {
        if (not m_shape.active)
            throw std::runtime_error("bezierVertex() called without a matching beginShape()");
        if (m_shape.positions.empty())
            throw std::runtime_error("bezierVertex() requires a preceding vertex() call");

        const float2 p0 = m_shape.positions.back();
        const float2 p1 {controlX1, controlY1};
        const float2 p2 {controlX2, controlY2};
        const float2 p3 {x, y};

        const float length = detail::distance2(p0, p1) + detail::distance2(p1, p2) + detail::distance2(p2, p3);
        const int segments = detail::curveSegmentCount(length);
        for (int i = 1; i <= segments; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(segments);
            appendShapeVertex(detail::cubicBezierPoint(p0, p1, p2, p3, t), {0.0f, 0.0f});
        }
    }

    void Graphics::quadraticVertex(float controlX, float controlY, float x, float y)
    {
        if (not m_shape.active)
            throw std::runtime_error("quadraticVertex() called without a matching beginShape()");
        if (m_shape.positions.empty())
            throw std::runtime_error("quadraticVertex() requires a preceding vertex() call");

        const float2 p0 = m_shape.positions.back();
        const float2 p1 {controlX, controlY};
        const float2 p2 {x, y};

        const float length = detail::distance2(p0, p1) + detail::distance2(p1, p2);
        const int segments = detail::curveSegmentCount(length);
        for (int i = 1; i <= segments; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(segments);
            appendShapeVertex(detail::quadraticBezierPoint(p0, p1, p2, t), {0.0f, 0.0f});
        }
    }

    void Graphics::curveVertex(float x, float y)
    {
        if (not m_shape.active)
            throw std::runtime_error("curveVertex() called without a matching beginShape()");

        m_shape.curveControlPoints.push_back({x, y});
        if (m_shape.curveControlPoints.size() < 4)
            return;

        const size_t n = m_shape.curveControlPoints.size();
        const float2& p0 = m_shape.curveControlPoints[n - 4];
        const float2& p1 = m_shape.curveControlPoints[n - 3];
        const float2& p2 = m_shape.curveControlPoints[n - 2];
        const float2& p3 = m_shape.curveControlPoints[n - 1];

        // Converts the Catmull-Rom segment between p1 and p2 (with p0/p3 as tangent controls) into an
        // equivalent cubic Bezier, so it can be subdivided with the same cubicBezierPoint() used above.
        // `tightness` == 0 reproduces the standard (uniform) Catmull-Rom spline; increasing it towards 1
        // pulls the curve straight through p1/p2 as p5.js's curveTightness() does.
        const float s = (1.0f - peekState().curveTightness) / 6.0f;
        const float2 b0 = p1;
        const float2 b1 {p1.x + (p2.x - p0.x) * s, p1.y + (p2.y - p0.y) * s};
        const float2 b2 {p2.x - (p3.x - p1.x) * s, p2.y - (p3.y - p1.y) * s};
        const float2 b3 = p2;

        if (m_shape.positions.empty())
            appendShapeVertex(b0, {0.0f, 0.0f});

        const float length = detail::distance2(b0, b1) + detail::distance2(b1, b2) + detail::distance2(b2, b3);
        const int segments = detail::curveSegmentCount(length);
        for (int i = 1; i <= segments; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(segments);
            appendShapeVertex(detail::cubicBezierPoint(b0, b1, b2, b3, t), {0.0f, 0.0f});
        }
    }

    void Graphics::endShape(bool close)
    {
        if (not m_shape.active)
            throw std::runtime_error("endShape() called without a matching beginShape()");

        const DrawState& state = peekState();
        const ShapeBuilder shape = std::move(m_shape);
        m_shape = ShapeBuilder {};

        if (state.isFillEnabled)
            submitFillMesh(shape.mode, shape.positions, shape.texCoords, shape.fillColors, state);

        if (state.isStrokeEnabled) {
            switch (shape.mode) {
                case ShapeMode::points:
                    for (size_t i = 0; i < shape.positions.size(); ++i)
                        submitPoint(shape.positions[i], shape.strokeColors[i], state);
                    break;

                case ShapeMode::lines:
                    for (size_t i = 0; i + 2 <= shape.positions.size(); i += 2) {
                        const float2 positions[2] = {shape.positions[i], shape.positions[i + 1]};
                        const float2 texCoords[2] = {shape.texCoords[i], shape.texCoords[i + 1]};
                        const color_t colors[2] = {shape.strokeColors[i], shape.strokeColors[i + 1]};
                        submitStroke(positions, texCoords, colors, false, state);
                    }
                    break;

                default:
                    submitStroke(shape.positions, shape.texCoords, shape.strokeColors, close, state);
                    break;
            }
        }
    }

    void Graphics::bezier(float x1, float y1, float controlX1, float controlY1, float controlX2, float controlY2, float x2, float y2)
    {
        // bezier()/curve() only ever stroke, matching p5.js; noFill() is scoped to this call via
        // pushState()/popState() so it doesn't leak into the caller's fill state.
        pushState();
        noFill();
        beginShape(ShapeMode::polygon);
        vertex(x1, y1);
        bezierVertex(controlX1, controlY1, controlX2, controlY2, x2, y2);
        endShape();
        popState();
    }

    void Graphics::curve(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
    {
        pushState();
        noFill();
        beginShape(ShapeMode::polygon);
        curveVertex(x1, y1);
        curveVertex(x2, y2);
        curveVertex(x3, y3);
        curveVertex(x4, y4);
        endShape();
        popState();
    }

    void Graphics::imageUVMode(TextureUVMode mode)
    {
        DrawState& state = peekState();
        state.textureUVMode = mode;
    }

    void Graphics::image(std::shared_ptr<Texture> texture, float left, float top, float width, float height)
    {
        image(texture, left, top, width, height, 0.0f, 0.0f, 1.0f, 1.0f);
    }

    void Graphics::image(std::shared_ptr<Texture> texture, float left, float top, float width, float height, float u1, float v1, float u2, float v2)
    {
        assert(texture != nullptr and "image() called with null texture");

        const DrawState& state = peekState();
        const TextureUVMode uvMode = state.textureUVMode;
        const color_t tintColor = state.tintColor;

        const auto [texWidth, texHeight] = texture->getSize();
        const float invTextureWidth = 1.0f / static_cast<float>(texWidth);
        const float invTextureHeight = 1.0f / static_cast<float>(texHeight);

        switch (uvMode) {
            case TextureUVMode::normalized:
                break;

            case TextureUVMode::pixel:
                u1 *= invTextureWidth;
                v1 *= invTextureHeight;
                u2 *= invTextureWidth;
                v2 *= invTextureHeight;
                break;

            default:
                throw std::runtime_error("imageUVMode() called with unknown mode");
        }

        const float2 positions[4] = {
            {left, top},
            {left + width, top},
            {left + width, top + height},
            {left, top + height},
        };

        const float2 texCoords[4] = {
            {u1, v1},
            {u2, v1},
            {u2, v2},
            {u1, v2},
        };

        submitQuad(positions, texCoords, tintColor, state, texture);
    }
} // namespace p5
