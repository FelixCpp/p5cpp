#include <p5cpp/graphics/tessellators.hpp>
#include <p5cpp/graphics/canvas.hpp>
#include <p5cpp/graphics/text_layout.hpp>
#include <p5cpp/graphics/dejavu_sans.hpp>
#include <p5cpp/graphics/default_shaders.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace p5
{
    namespace detail
    {
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

        // Glyph atlas cells store plain 8-bit AA coverage (see font.cpp), not a signed distance field --
        // same architecture as Processing's PFont/FontTexture. Coverage is read straight through as
        // alpha; the GPU's own bilinear filtering on the atlas texture (see loadTexture())
        // is the only antialiasing applied when a glyph is displayed larger or smaller than the size it
        // was baked at.
        inline static constexpr std::string_view defaultTextFragmentShaderSource = R"(
            #version 410

            layout (location = 0) out vec4 o_FragColor;

            in vec2 v_TexCoord;
            in vec4 v_Color;

            uniform sampler2D u_Texture;

            void main()
            {
                float coverage = texture(u_Texture, v_TexCoord).r;
                o_FragColor = vec4(v_Color.rgb, v_Color.a * coverage);
            }
        )";

        inline static constexpr size_t MAX_VERTICES = 100'000;
        inline static constexpr size_t MAX_INDICES = 150'000;
        inline static constexpr size_t TEXT_MESH_CHUNK_GLYPHS = 512;

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

        int arcSegmentCount(float radiusX, float radiusY, float angleSpan)
        {
            constexpr float twoPi = 2.0f * std::numbers::pi_v<float>;
            const float fraction = std::clamp(std::abs(angleSpan) / twoPi, 0.0f, 1.0f);
            const int segments = static_cast<int>(std::ceil(static_cast<float>(ellipseSegmentCount(radiusX, radiusY)) * fraction));
            return std::clamp(segments, 2, 256);
        }

        void buildArcPoints(float centerX, float centerY, float radiusX, float radiusY, float startAngle, float stopAngle, std::vector<float2>& positions, std::vector<float2>& texCoords)
        {
            const int segments = arcSegmentCount(radiusX, radiusY, stopAngle - startAngle);
            positions.resize(segments + 1);
            texCoords.resize(segments + 1);

            for (int i = 0; i <= segments; ++i) {
                const float angle = std::lerp(startAngle, stopAngle, static_cast<float>(i) / static_cast<float>(segments));
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
    Canvas::Canvas()
        : m_stateStack(),
          m_matrixStack(),
          m_renderer(Renderer::create(detail::MAX_VERTICES, detail::MAX_INDICES)),
          m_defaultFillShader(loadShaderFromMemory(detail::defaultVertexShaderSource, detail::defaultFragmentShaderSource).value()),
          m_defaultTextShader(loadShaderFromMemory(detail::defaultVertexShaderSource, detail::defaultTextFragmentShaderSource).value()),
          m_defaultTexture(loadTexture(1, 1, std::array<uint8_t, 4> {255, 255, 255, 255}).value()),
          m_defaultFont(loadFont(std::span<const uint8_t> {DejaVuSans_ttf, DejaVuSans_ttf_len}).value())
    {
    }

    void Canvas::push(const bool extend)
    {
        pushState(extend);
        pushMatrix(extend);
    }

    void Canvas::pop()
    {
        popMatrix();
        popState();
    }

    void Canvas::pushGraphics(Graphics graphics, const bool extend)
    {
        m_renderer->end();
        m_renderer->begin(graphics);
        m_graphicsStack.push(std::move(graphics));
        push(extend);
    }

    void Canvas::popGraphics()
    {
        pop();
        m_graphicsStack.pop();

        m_renderer->end();
        if (const Graphics newGraphics = m_graphicsStack.peek(); newGraphics.isValid()) {
            m_renderer->begin(newGraphics);
        }
    }

    Graphics Canvas::peekGraphics() const
    {
        return m_graphicsStack.peek();
    }

    uint2 Canvas::getGraphicsSize() const
    {
        static constexpr uint2 kFallbackSize {0, 0};

        const Graphics graphics = m_graphicsStack.peek();
        if (not graphics.isValid()) {
            error("getGraphicsSize() called with no graphics pushed");
            return kFallbackSize;
        }

        return graphics.size;
    }

    void Canvas::flush()
    {
        m_renderer->flush();
    }

    Pixels Canvas::loadPixels()
    {
        const Graphics graphics = m_graphicsStack.peek();
        if (not graphics.isValid()) {
            error("loadPixels() called with no graphics pushed");
            return {};
        }

        flush();
        return graphics.loadPixels();
    }

    void Canvas::updatePixels(const Pixels& pixels)
    {
        Graphics graphics = m_graphicsStack.peek();
        if (not graphics.isValid()) {
            error("updatePixels() called with no graphics pushed");
            return;
        }

        graphics.updatePixels(pixels);
    }

    void Canvas::pushState(const bool extend)
    {
        m_stateStack.push(extend ? peekState() : DrawState {});
    }

    void Canvas::popState()
    {
        m_stateStack.pop();
    }

    DrawState& Canvas::peekState()
    {
        return m_stateStack.peek();
    }

    void Canvas::pushMatrix(const bool extend)
    {
        m_matrixStack.push(extend ? peekMatrix() : identityMatrix());
    }

    void Canvas::popMatrix()
    {
        m_matrixStack.pop();
    }

    matrix4x4& Canvas::peekMatrix()
    {
        return m_matrixStack.peek();
    }

    void Canvas::applyMatrix(const matrix4x4& matrix)
    {
        m_matrixStack.set(m_matrixStack.peek() * matrix);
    }

    void Canvas::setMatrix(const matrix4x4& matrix)
    {
        m_matrixStack.set(matrix);
    }

    void Canvas::translate(float x, float y)
    {
        applyMatrix(translationMatrix(x, y));
    }

    void Canvas::scale(float x, float y)
    {
        applyMatrix(scalingMatrix(x, y));
    }

    void Canvas::rotate(float radians)
    {
        applyMatrix(rotationMatrix(radians));
    }

    void Canvas::fill(color_t color)
    {
        DrawState& state = peekState();
        state.isFillEnabled = true;
        state.fillColor = color;
    }

    void Canvas::noFill()
    {
        DrawState& state = peekState();
        state.isFillEnabled = false;
    }

    void Canvas::stroke(color_t color)
    {
        DrawState& state = peekState();
        state.isStrokeEnabled = true;
        state.strokeColor = color;
    }

    void Canvas::noStroke()
    {
        DrawState& state = peekState();
        state.isStrokeEnabled = false;
    }

    void Canvas::strokeWeight(float weight)
    {
        DrawState& state = peekState();
        state.strokeWeight = weight;
    }

    void Canvas::strokeCap(StrokeCap cap)
    {
        DrawState& state = peekState();
        state.strokeCap = cap;
    }

    void Canvas::strokeJoin(StrokeJoin join)
    {
        DrawState& state = peekState();
        state.strokeJoin = join;
    }

    void Canvas::strokeMiterLimit(float limit)
    {
        DrawState& state = peekState();
        state.strokeMiterLimit = limit;
    }

    void Canvas::strokeRoundJoinThreshold(float threshold)
    {
        DrawState& state = peekState();
        state.strokeRoundJoinThreshold = threshold;
    }

    void Canvas::curveTightness(float tightness)
    {
        DrawState& state = peekState();
        state.curveTightness = tightness;
    }

    void Canvas::tint(color_t color)
    {
        DrawState& state = peekState();
        state.tintColor = color;
    }

    void Canvas::noTint()
    {
        tint(rgba(255, 255, 255, 255));
    }

    void Canvas::blendMode(const BlendMode& blendMode)
    {
        DrawState& state = peekState();
        state.blendMode = blendMode;
    }

    void Canvas::clip(float x, float y, float width, float height)
    {
        DrawState& state = peekState();
        state.clipRect = rect2f {x, y, width, height};
    }

    void Canvas::noClip()
    {
        DrawState& state = peekState();
        state.clipRect = std::nullopt;
    }

    void Canvas::shader(Shader shader)
    {
        DrawState& state = peekState();
        state.shader = shader;
    }

    void Canvas::noShader()
    {
        shader(Shader {});
    }

    Shader Canvas::resolveActiveShader(const Shader& fallback)
    {
        DrawState& state = peekState();
        if (state.shader.isValid()) {
            return state.shader;
        }

        return fallback;
    }

    Texture Canvas::resolveActiveTexture(const Texture& texture)
    {
        if (texture.isValid()) {
            return texture;
        }

        return m_defaultTexture;
    }

    float2 Canvas::applyTransform(const float2& point) const
    {
        return p5::transformPoint(m_matrixStack.peek(), point);
    }

    void Canvas::submitQuad(const std::span<const float2, 4>& positions, const std::span<const float2, 4>& texCoords, color_t color, const DrawState& state, const Texture& texture)
    {
        const float4 col = detail::toFloat4(color);
        const float4 colors[4] = {col, col, col, col};

        Renderer::Writer writer = m_renderer->write();
        tesselate_quad(writer, positions, texCoords, colors);
        m_renderer->finish(writer, state.blendMode, state.clipRect, state.textureFilter, state.textureWrap, resolveActiveTexture(texture), resolveActiveShader(m_defaultFillShader));
    }

    void Canvas::submitStroke(const std::span<const float2>& positions, bool closed, color_t color, const DrawState& state)
    {
        const std::vector<float2> texCoords(positions.size(), float2 {0.0f, 0.0f});
        const std::vector<color_t> colors(positions.size(), color);
        submitStroke(positions, texCoords, colors, closed, state);
    }

    void Canvas::submitStroke(const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const color_t>& colors, bool closed, const DrawState& state)
    {
        std::vector<float4> convertedColors(colors.size());
        std::ranges::transform(colors, convertedColors.begin(), detail::toFloat4);

        Renderer::Writer writer = m_renderer->write();
        tesselate_path(writer, positions, texCoords, convertedColors, state.strokeWeight, state.strokeCap, state.strokeJoin, state.strokeMiterLimit, state.strokeRoundJoinThreshold, closed);
        m_renderer->finish(writer, state.blendMode, state.clipRect, state.textureFilter, state.textureWrap, resolveActiveTexture(), resolveActiveShader(m_defaultFillShader));
    }

    void Canvas::submitFillMesh(ShapeMode mode, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const color_t>& colors, const DrawState& state)
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
        m_renderer->finish(writer, state.blendMode, state.clipRect, state.textureFilter, state.textureWrap, resolveActiveTexture(), resolveActiveShader(m_defaultFillShader));
    }

    void Canvas::submitTextMesh(const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const color_t>& colors, const Texture& atlasTexture, const DrawState& state)
    {
        std::vector<float4> convertedColors(colors.size());
        std::ranges::transform(colors, convertedColors.begin(), detail::toFloat4);

        Renderer::Writer writer = m_renderer->write();
        tesselate_quads(writer, positions, texCoords, convertedColors);
        // Filter/wrap are forced (not state.textureFilter/textureWrap): linear sampling is a correctness
        // requirement for glyph scaling (see font.cpp -- each atlas cell is baked once and reused,
        // bilinear-scaled, for every requested textSize()), not a style choice like it is for
        // image()'s user textures.
        m_renderer->finish(writer, state.blendMode, state.clipRect, TextureFilter::linear, TextureWrap::clampToEdge, atlasTexture, resolveActiveShader(m_defaultTextShader));
    }

    void Canvas::background(color_t color)
    {
        const auto [width, height] = getGraphicsSize();
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

    void Canvas::rect(float x, float y, float width, float height)
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

    void Canvas::rect(float x, float y, float width, float height, const BorderRadius& borderRadius)
    {
        const DrawState& state = peekState();
        ShapeBuilder builder;
        builder.beginShape(ShapeMode::polygon);
        detail::buildRoundedRectPoints(x, y, width, height, borderRadius, state.fillColor, state.strokeColor, m_matrixStack.peek(), builder);
        submitBuiltShape(builder.endShape(), true);
    }

    void Canvas::square(float x, float y, float size)
    {
        rect(x, y, size, size);
    }

    void Canvas::ellipse(float x, float y, float radiusX, float radiusY)
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

    void Canvas::circle(float x, float y, float radius)
    {
        ellipse(x, y, radius, radius);
    }

    void Canvas::arc(float centerX, float centerY, float radiusX, float radiusY, float startAngle, float stopAngle, ArcMode mode)
    {
        constexpr float twoPi = 2.0f * std::numbers::pi_v<float>;
        if (stopAngle < startAngle) {
            stopAngle += twoPi * std::ceil((startAngle - stopAngle) / twoPi);
        }
        stopAngle = std::min(stopAngle, startAngle + twoPi);

        std::vector<float2> arcPositions;
        std::vector<float2> arcTexCoords;
        detail::buildArcPoints(centerX, centerY, radiusX, radiusY, startAngle, stopAngle, arcPositions, arcTexCoords);

        const DrawState& state = peekState();

        // OPEN fills the same pie wedge as PIE, but only strokes the arc itself (no radii/chord).
        const bool includeCenterInFill = mode == ArcMode::pie or mode == ArcMode::open;
        const bool closeStroke = mode == ArcMode::pie or mode == ArcMode::chord;

        if (state.isFillEnabled) {
            std::vector<float2> fillPositions;
            std::vector<float2> fillTexCoords;
            if (includeCenterInFill) {
                fillPositions.reserve(arcPositions.size() + 1);
                fillTexCoords.reserve(arcTexCoords.size() + 1);
                fillPositions.push_back(applyTransform({centerX, centerY}));
                fillTexCoords.push_back({0.5f, 0.5f});
            }
            for (size_t i = 0; i < arcPositions.size(); ++i) {
                fillPositions.push_back(applyTransform(arcPositions[i]));
                fillTexCoords.push_back(arcTexCoords[i]);
            }

            const std::vector<color_t> fillColors(fillPositions.size(), state.fillColor);
            submitFillMesh(ShapeMode::triangleFan, fillPositions, fillTexCoords, fillColors, state);
        }

        if (state.isStrokeEnabled) {
            std::vector<float2> strokePositions;
            strokePositions.reserve(arcPositions.size() + 1);
            if (mode == ArcMode::pie) {
                strokePositions.push_back(applyTransform({centerX, centerY}));
            }
            for (const float2& p : arcPositions) {
                strokePositions.push_back(applyTransform(p));
            }

            submitStroke(strokePositions, closeStroke, state.strokeColor, state);
        }
    }

    void Canvas::line(float x1, float y1, float x2, float y2)
    {
        const DrawState& state = peekState();
        if (not state.isStrokeEnabled)
            return;

        const float2 positions[2] = {applyTransform({x1, y1}), applyTransform({x2, y2})};
        submitStroke(positions, false, state.strokeColor, state);
    }

    void Canvas::triangle(float x1, float y1, float x2, float y2, float x3, float y3)
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

    void Canvas::submitPoint(const float2& position, color_t color, const DrawState& state)
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

    void Canvas::point(float x, float y)
    {
        const DrawState& state = peekState();
        if (not state.isStrokeEnabled)
            return;

        submitPoint(applyTransform({x, y}), state.strokeColor, state);
    }

    void Canvas::beginShape(ShapeMode mode)
    {
        m_shape.beginShape(mode);
    }

    void Canvas::vertex(float x, float y)
    {
        const DrawState& state = peekState();
        const float2 p = applyTransform({x, y});
        m_shape.vertex(p.x, p.y, state.fillColor, state.strokeColor);
    }

    void Canvas::vertex(float x, float y, float u, float v)
    {
        const DrawState& state = peekState();
        const float2 p = applyTransform({x, y});
        m_shape.vertex(p.x, p.y, u, v, state.fillColor, state.strokeColor);
    }

    void Canvas::bezierVertex(float controlX1, float controlY1, float controlX2, float controlY2, float x, float y)
    {
        const float2 c1 = applyTransform({controlX1, controlY1});
        const float2 c2 = applyTransform({controlX2, controlY2});
        const float2 p = applyTransform({x, y});
        m_shape.bezierVertex(c1.x, c1.y, c2.x, c2.y, p.x, p.y);
    }

    void Canvas::quadraticVertex(float controlX, float controlY, float x, float y)
    {
        const float2 c = applyTransform({controlX, controlY});
        const float2 p = applyTransform({x, y});
        m_shape.quadraticVertex(c.x, c.y, p.x, p.y);
    }

    void Canvas::curveVertex(float x, float y)
    {
        const DrawState& state = peekState();
        const float2 p = applyTransform({x, y});
        m_shape.curveVertex(p.x, p.y, state.curveTightness, state.fillColor, state.strokeColor);
    }

    void Canvas::endShape(bool close)
    {
        submitBuiltShape(m_shape.endShape(), close);
    }

    void Canvas::submitBuiltShape(const BuiltShape& shape, bool close)
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

    void Canvas::bezier(float x1, float y1, float controlX1, float controlY1, float controlX2, float controlY2, float x2, float y2)
    {
        pushState(true);
        noFill();
        beginShape(ShapeMode::path);
        vertex(x1, y1);
        bezierVertex(controlX1, controlY1, controlX2, controlY2, x2, y2);
        endShape();
        popState();
    }

    void Canvas::curve(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
    {
        pushState(true);
        noFill();
        beginShape(ShapeMode::path);
        curveVertex(x1, y1);
        curveVertex(x2, y2);
        curveVertex(x3, y3);
        curveVertex(x4, y4);
        endShape();
        popState();
    }

    void Canvas::textureUVMode(TextureUVMode mode)
    {
        DrawState& state = peekState();
        state.textureUVMode = mode;
    }

    void Canvas::textureFilter(TextureFilter filter)
    {
        DrawState& state = peekState();
        state.textureFilter = filter;
    }

    void Canvas::textureWrap(TextureWrap wrap)
    {
        DrawState& state = peekState();
        state.textureWrap = wrap;
    }

    void Canvas::image(Texture texture, float left, float top, float width, float height)
    {
        image(texture, left, top, width, height, 0.0f, 0.0f, 1.0f, 1.0f);
    }

    void Canvas::image(Texture texture, float left, float top, float width, float height, float u1, float v1, float u2, float v2)
    {
        if (not texture.isValid()) {
            error("image() called with an invalid texture");
            return;
        }

        const DrawState& state = peekState();
        const TextureUVMode uvMode = state.textureUVMode;
        const color_t tintColor = state.tintColor;

        const auto [texWidth, texHeight] = texture.size;
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
                error("image() called with an unknown TextureUVMode");
                return;
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

    void Canvas::textFont(Font font)
    {
        DrawState& state = peekState();
        state.textFont = font;
    }

    void Canvas::noTextFont()
    {
        textFont(Font {});
    }

    void Canvas::textSize(float pixels)
    {
        DrawState& state = peekState();
        state.textSize = pixels;
    }

    void Canvas::textAlign(TextAlignment alignment)
    {
        DrawState& state = peekState();
        state.textAlignment = alignment;
    }

    void Canvas::textWrap(TextWrap wrap)
    {
        DrawState& state = peekState();
        state.textWrap = wrap;
    }

    void Canvas::textLeading(float pixels)
    {
        DrawState& state = peekState();
        state.textLeadingOverride = pixels;
    }

    void Canvas::noTextLeading()
    {
        DrawState& state = peekState();
        state.textLeadingOverride = std::nullopt;
    }

    void Canvas::textLetterSpacing(float pixels)
    {
        DrawState& state = peekState();
        state.textLetterSpacing = pixels;
    }

    void Canvas::text(std::string_view str, float x, float y, float maxWidth, float maxHeight)
    {
        DrawState& state = peekState();
        const Font& font = state.textFont.isValid() ? state.textFont : m_defaultFont;
        const float scale = state.textSize / font.getUnitsPerEm();

        const detail::LineLayout layout = detail::layoutLines(font, state.textSize, str, state.textWrap, maxWidth, state.textLetterSpacing);
        const size_t numLines = layout.lines.size();

        const detail::TextBlockLayout blockLayout = detail::computeTextBlockLayout(font, layout, scale, state.textAlignment, {x, y}, state.textLeadingOverride);
        const float leading = blockLayout.leading;
        const float blockTop = blockLayout.blockTop;
        const float2 blockOrigin = blockLayout.blockOrigin;
        const float blockWidth = blockLayout.blockWidth;

        size_t visibleLines = numLines;
        if (maxHeight > 0.0f) {
            visibleLines = 0;
            for (size_t i = 0; i < numLines; ++i) {
                const float baselineOffset = blockTop + static_cast<float>(i) * leading;
                if (baselineOffset > maxHeight and visibleLines > 0) {
                    break;
                }
                ++visibleLines;
            }
        }

        std::vector<float2> positions;
        std::vector<float2> texCoords;
        std::vector<color_t> colors;
        size_t pendingGlyphs = 0;

        const auto flushGlyphChunk = [&]() {
            if (not positions.empty()) {
                submitTextMesh(positions, texCoords, colors, font.getAtlasTexture(), state);
                positions.clear();
                texCoords.clear();
                colors.clear();
                pendingGlyphs = 0;
            }
        };

        for (size_t lineIndex = 0; lineIndex < visibleLines; ++lineIndex) {
            const detail::ShapedLine& line = layout.lines[lineIndex];
            const float lineWidthPixels = line.width * scale;
            float penX = blockOrigin.x + detail::lineHorizontalOffset(blockWidth, lineWidthPixels, state.textAlignment);
            const float penYBaseline = blockOrigin.y + blockTop + static_cast<float>(lineIndex) * leading;
            float penY = penYBaseline;

            for (const ShapedGlyph& g : line.glyphs) {
                const GlyphMetrics& metrics = font.getGlyphMetrics(g.glyphIndex);
                if (metrics.hasOutline) {
                    const float2 glyphOrigin {penX + g.xOffset * scale, penY - g.yOffset * scale};
                    const float2 quadTopLeft {glyphOrigin.x + metrics.bounds.left * scale, glyphOrigin.y - metrics.bounds.top * scale};
                    const float2 quadSize {metrics.bounds.width * scale, metrics.bounds.height * scale};

                    const float2 corners[4] = {
                        applyTransform({quadTopLeft.x, quadTopLeft.y}),
                        applyTransform({quadTopLeft.x + quadSize.x, quadTopLeft.y}),
                        applyTransform({quadTopLeft.x + quadSize.x, quadTopLeft.y + quadSize.y}),
                        applyTransform({quadTopLeft.x, quadTopLeft.y + quadSize.y}),
                    };
                    const float2 uv[4] = {
                        {metrics.uvRect.left, metrics.uvRect.top},
                        {metrics.uvRect.left + metrics.uvRect.width, metrics.uvRect.top},
                        {metrics.uvRect.left + metrics.uvRect.width, metrics.uvRect.top + metrics.uvRect.height},
                        {metrics.uvRect.left, metrics.uvRect.top + metrics.uvRect.height},
                    };

                    positions.insert(positions.end(), std::begin(corners), std::end(corners));
                    texCoords.insert(texCoords.end(), std::begin(uv), std::end(uv));
                    colors.insert(colors.end(), 4, state.fillColor);

                    if (++pendingGlyphs >= detail::TEXT_MESH_CHUNK_GLYPHS) {
                        flushGlyphChunk();
                    }
                }

                penX += g.xAdvance * scale + state.textLetterSpacing;
                penY -= g.yAdvance * scale;
            }
        }

        flushGlyphChunk();
    }

    float Canvas::textWidth(std::string_view str)
    {
        DrawState& state = peekState();
        const Font& font = state.textFont.isValid() ? state.textFont : m_defaultFont;
        return p5::textWidth(font, state.textSize, str, state.textLetterSpacing);
    }

    rect2f Canvas::textBounds(std::string_view str, float maxWidth)
    {
        DrawState& state = peekState();
        const Font& font = state.textFont.isValid() ? state.textFont : m_defaultFont;
        return p5::textBounds(font, state.textSize, str, state.textWrap, maxWidth, state.textLetterSpacing);
    }

    std::vector<TextPoint> Canvas::textToPoints(std::string_view str, float x, float y, const TextToPointsOptions& options)
    {
        DrawState& state = peekState();
        const Font& font = options.font.has_value() ? *options.font : (state.textFont.isValid() ? state.textFont : m_defaultFont);
        const float effectiveSize = options.size.value_or(state.textSize);
        const float effectiveLetterSpacing = options.letterSpacing.value_or(state.textLetterSpacing);
        const float scale = effectiveSize / font.getUnitsPerEm();

        const detail::LineLayout layout = detail::layoutLines(font, effectiveSize, str, TextWrap::none, 0.0f, effectiveLetterSpacing);
        const detail::TextBlockLayout blockLayout = detail::computeTextBlockLayout(font, layout, scale, state.textAlignment, {x, y}, state.textLeadingOverride);

        std::vector<TextPoint> result;
        uint32_t nextContourIndex = 0;
        for (size_t lineIndex = 0; lineIndex < layout.lines.size(); ++lineIndex) {
            const detail::ShapedLine& line = layout.lines[lineIndex];
            const float lineWidthPixels = line.width * scale;
            const float penX = blockLayout.blockOrigin.x + detail::lineHorizontalOffset(blockLayout.blockWidth, lineWidthPixels, state.textAlignment);
            const float penY = blockLayout.blockOrigin.y + blockLayout.blockTop + static_cast<float>(lineIndex) * blockLayout.leading;
            detail::appendLineToPoints(font, line, scale, penX, penY, effectiveLetterSpacing, options, result, nextContourIndex);
        }

        return result;
    }
} // namespace p5
