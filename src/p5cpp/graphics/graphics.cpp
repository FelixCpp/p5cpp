#include <p5cpp/graphics/tessellators.hpp>
#include <p5cpp/graphics/graphics.hpp>
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

        inline static constexpr std::string_view defaultTextFragmentShaderSource = R"(
            #version 410

            layout (location = 0) out vec4 o_FragColor;

            in vec2 v_TexCoord;
            in vec4 v_Color;

            uniform sampler2D u_Texture;

            void main()
            {
                float distance = texture(u_Texture, v_TexCoord).r;
                float smoothing = fwidth(distance) * 0.75;
                float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
                o_FragColor = vec4(v_Color.rgb, v_Color.a * alpha);
            }
        )";

        // Per-frame batch capacity: every draw call between Renderer::begin() and the next
        // flush() accumulates into this one fixed buffer (see Renderer::appendVertex/appendIndex),
        // so it must cover a whole frame's worth of geometry, not just one shape. A handful of
        // multi-hundred-point polylines/rounded rects in a single dashboard-style frame can run
        // into the tens of thousands of indices (stroked polylines alone cost roughly 15 indices
        // per interior point for miter joins), so the previous 4096/6144 ceiling was only really
        // sized for the simplest sketches and threw std::runtime_error well within normal use.
        inline static constexpr size_t MAX_VERTICES = 65536;
        inline static constexpr size_t MAX_INDICES = 98304;

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
    Graphics::Graphics()
        : m_stateStack(),
          m_matrixStack(),
          m_renderer(Renderer::create(detail::MAX_VERTICES, detail::MAX_INDICES)),
          m_defaultFillShader(loadShaderFromMemory(detail::defaultVertexShaderSource, detail::defaultFragmentShaderSource)),
          m_defaultTextShader(loadShaderFromMemory(detail::defaultVertexShaderSource, detail::defaultTextFragmentShaderSource)),
          m_defaultTexture(loadTextureFromMemory(1, 1, std::array<uint8_t, 4> {255, 255, 255, 255})),
          m_defaultFont(loadFontFromMemory({DejaVuSans_ttf, DejaVuSans_ttf_len}))
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
        static constexpr uint2 kFallbackSize {0, 0};

        const std::shared_ptr<Framebuffer> framebuffer = m_framebufferStack.peek();
        if (framebuffer == nullptr) {
            error("getFramebufferSize() called with no framebuffer pushed");
            return kFallbackSize;
        }

        return framebuffer->getSize();
    }

    void Graphics::flush()
    {
        m_renderer->flush();
    }

    Pixels Graphics::loadPixels()
    {
        const std::shared_ptr<Framebuffer> framebuffer = m_framebufferStack.peek();
        if (framebuffer == nullptr) {
            error("loadPixels() called with no framebuffer pushed");
            return {};
        }

        flush();
        return p5::loadPixels(*framebuffer);
    }

    void Graphics::updatePixels(const Pixels& pixels)
    {
        const std::shared_ptr<Framebuffer> framebuffer = m_framebufferStack.peek();
        if (framebuffer == nullptr) {
            error("updatePixels() called with no framebuffer pushed");
            return;
        }

        p5::updatePixels(*framebuffer, pixels);
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

    std::shared_ptr<Shader> Graphics::resolveActiveShader(const std::shared_ptr<Shader>& fallback)
    {
        DrawState& state = peekState();
        if (state.shader != nullptr) {
            return state.shader;
        }

        return fallback;
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
        m_renderer->finish(writer, state.blendMode, state.clipRect, state.textureFilter, state.textureWrap, resolveActiveTexture(texture), resolveActiveShader(m_defaultFillShader));
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
        m_renderer->finish(writer, state.blendMode, state.clipRect, state.textureFilter, state.textureWrap, resolveActiveTexture(), resolveActiveShader(m_defaultFillShader));
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
        m_renderer->finish(writer, state.blendMode, state.clipRect, state.textureFilter, state.textureWrap, resolveActiveTexture(), resolveActiveShader(m_defaultFillShader));
    }

    void Graphics::submitTextMesh(const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const color_t>& colors, const std::shared_ptr<Texture>& atlasTexture, const DrawState& state)
    {
        std::vector<float4> convertedColors(colors.size());
        std::ranges::transform(colors, convertedColors.begin(), detail::toFloat4);

        Renderer::Writer writer = m_renderer->write();
        tesselate_quads(writer, positions, texCoords, convertedColors);
        // Filter/wrap are forced (not state.textureFilter/textureWrap): linear sampling is a correctness
        // requirement of the SDF antialiasing, not a style choice like it is for image()'s user textures.
        m_renderer->finish(writer, state.blendMode, state.clipRect, TextureFilter::linear, TextureWrap::clampToEdge, atlasTexture, resolveActiveShader(m_defaultTextShader));
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

    void Graphics::arc(float centerX, float centerY, float radiusX, float radiusY, float startAngle, float stopAngle, ArcMode mode)
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

    void Graphics::textureUVMode(TextureUVMode mode)
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
            error("image() called with null texture");
            return;
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

    void Graphics::textFont(std::shared_ptr<Font> font)
    {
        DrawState& state = peekState();
        state.textFont = std::move(font);
    }

    void Graphics::noTextFont()
    {
        textFont(nullptr);
    }

    void Graphics::textSize(float pixels)
    {
        DrawState& state = peekState();
        state.textSize = pixels;
    }

    void Graphics::textAlign(TextAlignment alignment)
    {
        DrawState& state = peekState();
        state.textAlignment = alignment;
    }

    void Graphics::textWrap(TextWrap wrap)
    {
        DrawState& state = peekState();
        state.textWrap = wrap;
    }

    void Graphics::textLeading(float pixels)
    {
        DrawState& state = peekState();
        state.textLeadingOverride = pixels;
    }

    void Graphics::noTextLeading()
    {
        DrawState& state = peekState();
        state.textLeadingOverride = std::nullopt;
    }

    void Graphics::textLetterSpacing(float pixels)
    {
        DrawState& state = peekState();
        state.textLetterSpacing = pixels;
    }

    void Graphics::text(std::string_view str, float x, float y, float maxWidth, float maxHeight)
    {
        DrawState& state = peekState();
        Font& font = state.textFont != nullptr ? *state.textFont : *m_defaultFont;
        const float scale = state.textSize / font.getUnitsPerEm();
        const float leading = state.textLeadingOverride.value_or((font.getAscent() + font.getDescent() + font.getLineGap()) * scale);

        const detail::LineLayout layout = detail::layoutLines(font, state.textSize, str, state.textWrap, maxWidth, state.textLetterSpacing);
        const size_t numLines = layout.lines.size();

        float blockWidth = 0.0f;
        for (const detail::ShapedLine& line : layout.lines) {
            blockWidth = std::max(blockWidth, line.width * scale);
        }

        const float blockTop = font.getAscent() * scale;
        const float blockHeight = blockTop + static_cast<float>(numLines - 1) * leading + font.getDescent() * scale;

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

        float horizontalBlockOffset = 0.0f;
        switch (state.textAlignment) {
            case TextAlignment::topCenter:
            case TextAlignment::center:
            case TextAlignment::bottomCenter: horizontalBlockOffset = -blockWidth * 0.5f; break;
            case TextAlignment::topRight:
            case TextAlignment::centerRight:
            case TextAlignment::bottomRight: horizontalBlockOffset = -blockWidth; break;
            default: break; // *Left stays 0
        }

        float verticalBlockOffset = 0.0f;
        switch (state.textAlignment) {
            case TextAlignment::centerLeft:
            case TextAlignment::center:
            case TextAlignment::centerRight: verticalBlockOffset = -blockHeight * 0.5f; break;
            case TextAlignment::bottomLeft:
            case TextAlignment::bottomCenter:
            case TextAlignment::bottomRight: verticalBlockOffset = -blockHeight; break;
            default: break; // top* stays 0
        }

        const float2 blockOrigin {x + horizontalBlockOffset, y + verticalBlockOffset};

        const auto lineHorizontalOffset = [&](float lineWidthPixels) -> float {
            switch (state.textAlignment) {
                case TextAlignment::topCenter:
                case TextAlignment::center:
                case TextAlignment::bottomCenter: return (blockWidth - lineWidthPixels) * 0.5f;
                case TextAlignment::topRight:
                case TextAlignment::centerRight:
                case TextAlignment::bottomRight: return blockWidth - lineWidthPixels;
                default: return 0.0f; // *Left
            }
        };

        std::vector<float2> positions;
        std::vector<float2> texCoords;
        std::vector<color_t> colors;

        for (size_t lineIndex = 0; lineIndex < visibleLines; ++lineIndex) {
            const detail::ShapedLine& line = layout.lines[lineIndex];
            const float lineWidthPixels = line.width * scale;
            float penX = blockOrigin.x + lineHorizontalOffset(lineWidthPixels);
            const float penYBaseline = blockOrigin.y + blockTop + static_cast<float>(lineIndex) * leading;
            float penY = penYBaseline;

            for (const ShapedGlyph& g : line.glyphs) {
                const GlyphMetrics& metrics = font.getGlyphMetrics(g.glyphIndex);
                if (metrics.hasOutline) {
                    // HarfBuzz offsets/font design units use +y = up; screen space uses +y = down.
                    const float2 glyphOrigin {penX + g.xOffset * scale, penY - g.yOffset * scale};
                    const float2 quadTopLeft {glyphOrigin.x + metrics.bounds.x * scale, glyphOrigin.y - metrics.bounds.y * scale};
                    const float2 quadSize {metrics.bounds.width * scale, metrics.bounds.height * scale};

                    const float2 corners[4] = {
                        applyTransform({quadTopLeft.x, quadTopLeft.y}),
                        applyTransform({quadTopLeft.x + quadSize.x, quadTopLeft.y}),
                        applyTransform({quadTopLeft.x + quadSize.x, quadTopLeft.y + quadSize.y}),
                        applyTransform({quadTopLeft.x, quadTopLeft.y + quadSize.y}),
                    };
                    const float2 uv[4] = {
                        {metrics.uvRect.x, metrics.uvRect.y},
                        {metrics.uvRect.x + metrics.uvRect.width, metrics.uvRect.y},
                        {metrics.uvRect.x + metrics.uvRect.width, metrics.uvRect.y + metrics.uvRect.height},
                        {metrics.uvRect.x, metrics.uvRect.y + metrics.uvRect.height},
                    };

                    positions.insert(positions.end(), std::begin(corners), std::end(corners));
                    texCoords.insert(texCoords.end(), std::begin(uv), std::end(uv));
                    colors.insert(colors.end(), 4, state.fillColor);
                }

                penX += g.xAdvance * scale + state.textLetterSpacing;
                penY -= g.yAdvance * scale;
            }
        }

        if (not positions.empty()) {
            submitTextMesh(positions, texCoords, colors, font.getAtlasTexture(), state);
        }
    }

    float Graphics::textWidth(std::string_view str)
    {
        DrawState& state = peekState();
        Font& font = state.textFont != nullptr ? *state.textFont : *m_defaultFont;
        return p5::textWidth(font, state.textSize, str, state.textLetterSpacing);
    }

    rect2f Graphics::textBounds(std::string_view str, float maxWidth)
    {
        DrawState& state = peekState();
        Font& font = state.textFont != nullptr ? *state.textFont : *m_defaultFont;
        return p5::textBounds(font, state.textSize, str, state.textWrap, maxWidth, state.textLetterSpacing);
    }

} // namespace p5
