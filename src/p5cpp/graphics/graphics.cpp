#include <p5cpp/graphics/tessellators.hpp>
#include <p5cpp/graphics/graphics.hpp>

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

        void buildRoundedRectPoints(float left, float top, float width, float height, const BorderRadius& borderRadius, color_t fillColor, color_t strokeColor, const matrix4x4& transform, ShapeBuilder& builder)
        {
            const float right = left + width;
            const float bottom = top + height;
            const float halfWidth = width * 0.5f;
            const float halfHeight = height * 0.5f;

            const auto clampCorner = [&](const CornerRadius& corner) -> float2 {
                return {std::clamp(corner.radiusX, 0.0f, halfWidth), std::clamp(corner.radiusY, 0.0f, halfHeight)};
            };

            const float2 topLeftRadius = clampCorner(borderRadius.topLeft);
            const float2 topRightRadius = clampCorner(borderRadius.topRight);
            const float2 bottomRightRadius = clampCorner(borderRadius.bottomRight);
            const float2 bottomLeftRadius = clampCorner(borderRadius.bottomLeft);

            const auto addCorner = [&](float cornerX, float cornerY, float centerX, float centerY, float radiusX, float radiusY, float startAngle, float endAngle) {
                const auto addVertex = [&](float x, float y) {
                    const float2 transformed = transformPoint(transform, {x, y});
                    builder.vertex(transformed.x, transformed.y, (x - left) / width, (y - top) / height, fillColor, strokeColor);
                };

                if (radiusX <= 0.0f or radiusY <= 0.0f) {
                    addVertex(cornerX, cornerY);
                    return;
                }

                const int segments = std::max(ellipseSegmentCount(radiusX, radiusY) / 4, 2);
                for (int i = 0; i <= segments; ++i) {
                    const float t = std::lerp(startAngle, endAngle, static_cast<float>(i) / static_cast<float>(segments));
                    addVertex(centerX + std::cos(t) * radiusX, centerY + std::sin(t) * radiusY);
                }
            };

            constexpr float pi = std::numbers::pi_v<float>;

            addCorner(left, top, left + topLeftRadius.x, top + topLeftRadius.y, topLeftRadius.x, topLeftRadius.y, pi, 1.5f * pi);
            addCorner(right, top, right - topRightRadius.x, top + topRightRadius.y, topRightRadius.x, topRightRadius.y, 1.5f * pi, 2.0f * pi);
            addCorner(right, bottom, right - bottomRightRadius.x, bottom - bottomRightRadius.y, bottomRightRadius.x, bottomRightRadius.y, 0.0f, 0.5f * pi);
            addCorner(left, bottom, left + bottomLeftRadius.x, bottom - bottomLeftRadius.y, bottomLeftRadius.x, bottomLeftRadius.y, 0.5f * pi, pi);
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
        const std::shared_ptr<Framebuffer> framebuffer = m_framebufferStack.peek();
        if (framebuffer == nullptr) {
            throw std::runtime_error("getFramebufferSize() called with no framebuffer pushed");
        }

        return framebuffer->getSize();
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

    void Graphics::clip(float x, float y, float width, float height)
    {
        DrawState& state = peekState();
        state.clipRect = rect2f {x, y, width, height};
    }

    void Graphics::noClip()
    {
        DrawState& state = peekState();
        state.clipRect = std::nullopt;
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

    float2 Graphics::applyTransform(const float2& point) const
    {
        return p5::transformPoint(m_matrixStack.peek(), point);
    }

    void Graphics::submitQuad(const std::span<const float2, 4>& positions, const std::span<const float2, 4>& texCoords, color_t color, const DrawState& state, const std::shared_ptr<Texture>& texture)
    {
        const float4 col = detail::toFloat4(color);
        const float4 colors[4] = {col, col, col, col};

        Renderer::Writer writer = m_renderer->write();
        tesselate_quad(writer, positions, texCoords, colors);
        m_renderer->finish(writer, state.blendMode, state.clipRect, state.textureFilter, state.textureWrap, resolveActiveTexture(texture), resolveActiveShader());
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
        m_renderer->finish(writer, state.blendMode, state.clipRect, state.textureFilter, state.textureWrap, resolveActiveTexture(), resolveActiveShader());
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
            case ShapeMode::lines:
            case ShapeMode::path: return; // points/lines/path have no interior to fill
            case ShapeMode::polygon:
            default: tesselate_polygon(writer, positions, texCoords, convertedColors); break;
        }
        m_renderer->finish(writer, state.blendMode, state.clipRect, state.textureFilter, state.textureWrap, resolveActiveTexture(), resolveActiveShader());
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
        const DrawState& state = peekState();
        ShapeBuilder builder;
        builder.beginShape(ShapeMode::quads);
        const float2 topLeft = applyTransform({x, y});
        const float2 topRight = applyTransform({x + width, y});
        const float2 bottomRight = applyTransform({x + width, y + height});
        const float2 bottomLeft = applyTransform({x, y + height});
        builder.vertex(topLeft.x, topLeft.y, 0.0f, 0.0f, state.fillColor, state.strokeColor);
        builder.vertex(topRight.x, topRight.y, 1.0f, 0.0f, state.fillColor, state.strokeColor);
        builder.vertex(bottomRight.x, bottomRight.y, 1.0f, 1.0f, state.fillColor, state.strokeColor);
        builder.vertex(bottomLeft.x, bottomLeft.y, 0.0f, 1.0f, state.fillColor, state.strokeColor);
        submitBuiltShape(builder.endShape(), true);
    }

    void Graphics::rect(float x, float y, float width, float height, const BorderRadius& borderRadius)
    {
        const DrawState& state = peekState();
        ShapeBuilder builder;
        builder.beginShape(ShapeMode::polygon);
        detail::buildRoundedRectPoints(x, y, width, height, borderRadius, state.fillColor, state.strokeColor, m_matrixStack.peek(), builder);
        submitBuiltShape(builder.endShape(), true);
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
        ShapeBuilder builder;
        builder.beginShape(ShapeMode::triangleFan);
        for (size_t i = 0; i < positions.size(); ++i) {
            const float2 p = applyTransform(positions[i]);
            builder.vertex(p.x, p.y, texCoords[i].x, texCoords[i].y, state.fillColor, state.strokeColor);
        }
        submitBuiltShape(builder.endShape(), true);
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

        const float2 positions[2] = {applyTransform({x1, y1}), applyTransform({x2, y2})};
        submitStroke(positions, false, state.strokeColor, state);
    }

    void Graphics::triangle(float x1, float y1, float x2, float y2, float x3, float y3)
    {
        const DrawState& state = peekState();
        ShapeBuilder builder;
        builder.beginShape(ShapeMode::triangles);
        const float2 p1 = applyTransform({x1, y1});
        const float2 p2 = applyTransform({x2, y2});
        const float2 p3 = applyTransform({x3, y3});
        builder.vertex(p1.x, p1.y, 0.0f, 0.0f, state.fillColor, state.strokeColor);
        builder.vertex(p2.x, p2.y, 1.0f, 0.0f, state.fillColor, state.strokeColor);
        builder.vertex(p3.x, p3.y, 0.5f, 1.0f, state.fillColor, state.strokeColor);
        submitBuiltShape(builder.endShape(), true);
    }

    void Graphics::submitPoint(const float2& position, color_t color, const DrawState& state)
    {
        const float radius = std::max(state.strokeWeight, 1.0f) * 0.5f;
        const bool round = state.strokeCap.start == StrokeCapStyle::round or state.strokeCap.end == StrokeCapStyle::round;

        if (round) {
            std::vector<float2> positions;
            std::vector<float2> texCoords;
            detail::buildEllipsePoints(position.x, position.y, radius, radius, positions, texCoords);
            const std::vector<color_t> colors(positions.size(), color);
            submitFillMesh(ShapeMode::triangleFan, positions, texCoords, colors, state);
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

        submitPoint(applyTransform({x, y}), state.strokeColor, state);
    }

    void Graphics::beginShape(ShapeMode mode)
    {
        m_shape.beginShape(mode);
    }

    void Graphics::vertex(float x, float y)
    {
        const DrawState& state = peekState();
        const float2 p = applyTransform({x, y});
        m_shape.vertex(p.x, p.y, state.fillColor, state.strokeColor);
    }

    void Graphics::vertex(float x, float y, float u, float v)
    {
        const DrawState& state = peekState();
        const float2 p = applyTransform({x, y});
        m_shape.vertex(p.x, p.y, u, v, state.fillColor, state.strokeColor);
    }

    void Graphics::bezierVertex(float controlX1, float controlY1, float controlX2, float controlY2, float x, float y)
    {
        const float2 c1 = applyTransform({controlX1, controlY1});
        const float2 c2 = applyTransform({controlX2, controlY2});
        const float2 p = applyTransform({x, y});
        m_shape.bezierVertex(c1.x, c1.y, c2.x, c2.y, p.x, p.y);
    }

    void Graphics::quadraticVertex(float controlX, float controlY, float x, float y)
    {
        const float2 c = applyTransform({controlX, controlY});
        const float2 p = applyTransform({x, y});
        m_shape.quadraticVertex(c.x, c.y, p.x, p.y);
    }

    void Graphics::curveVertex(float x, float y)
    {
        const DrawState& state = peekState();
        const float2 p = applyTransform({x, y});
        m_shape.curveVertex(p.x, p.y, state.curveTightness, state.fillColor, state.strokeColor);
    }

    void Graphics::endShape(bool close)
    {
        submitBuiltShape(m_shape.endShape(), close);
    }

    void Graphics::submitBuiltShape(const BuiltShape& shape, bool close)
    {
        if (shape.vertexCount == 0) {
            return;
        }

        const DrawState& state = peekState();
        if (state.isFillEnabled) {
            submitFillMesh(shape.mode, shape.positions, shape.texCoords, shape.fillColors, state);
        }

        if (state.isStrokeEnabled) {
            switch (shape.mode) {
                case ShapeMode::points:
                    for (size_t i = 0; i < shape.vertexCount; ++i)
                        submitPoint(shape.positions[i], shape.strokeColors[i], state);
                    break;

                case ShapeMode::lines:
                    for (size_t i = 0; i + 2 <= shape.vertexCount; i += 2) {
                        const float2 positions[2] = {shape.positions[i], shape.positions[i + 1]};
                        const float2 texCoords[2] = {shape.texCoords[i], shape.texCoords[i + 1]};
                        const color_t colors[2] = {shape.strokeColors[i], shape.strokeColors[i + 1]};
                        submitStroke(positions, texCoords, colors, false, state);
                    }
                    break;

                case ShapeMode::path:
                default:
                    submitStroke(shape.positions, shape.texCoords, shape.strokeColors, close, state);
                    break;
            }
        }
    }

    void Graphics::bezier(float x1, float y1, float controlX1, float controlY1, float controlX2, float controlY2, float x2, float y2)
    {
        pushState();
        noFill();
        beginShape(ShapeMode::path);
        vertex(x1, y1);
        bezierVertex(controlX1, controlY1, controlX2, controlY2, x2, y2);
        endShape();
        popState();
    }

    void Graphics::curve(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
    {
        pushState();
        noFill();
        beginShape(ShapeMode::path);
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

    void Graphics::textureFilter(TextureFilter filter)
    {
        DrawState& state = peekState();
        state.textureFilter = filter;
    }

    void Graphics::textureWrap(TextureWrap wrap)
    {
        DrawState& state = peekState();
        state.textureWrap = wrap;
    }

    void Graphics::image(std::shared_ptr<Texture> texture, float left, float top, float width, float height)
    {
        image(texture, left, top, width, height, 0.0f, 0.0f, 1.0f, 1.0f);
    }

    void Graphics::image(std::shared_ptr<Texture> texture, float left, float top, float width, float height, float u1, float v1, float u2, float v2)
    {
        if (texture == nullptr) {
            throw std::runtime_error("image() called with null texture");
        }

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
            applyTransform({left, top}),
            applyTransform({left + width, top}),
            applyTransform({left + width, top + height}),
            applyTransform({left, top + height}),
        };

        const float2 texCoords[4] = {
            {u1, v2},
            {u2, v2},
            {u2, v1},
            {u1, v1},
        };

        submitQuad(positions, texCoords, tintColor, state, texture);
    }
} // namespace p5
