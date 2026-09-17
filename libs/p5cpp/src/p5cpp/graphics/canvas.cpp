#include <p5cpp/graphics/tessellators.hpp>
#include <p5cpp/graphics/canvas.hpp>
#include <p5cpp/graphics/dejavu_sans.hpp>
#include <p5cpp/graphics/default_shaders.hpp>
#include <p5cpp/graphics/primitive_geometry.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace p5
{
    namespace detail
    {
        inline static constexpr std::string_view defaultFragmentShaderSource = R"(
            @group(1) @binding(0) var u_Texture: texture_2d<f32>;
            @group(1) @binding(1) var u_Sampler: sampler;

            @fragment
            fn fs_main(
                @location(0) v_TexCoord: vec2f,
                @location(1) v_Color: vec4f,
            ) -> @location(0) vec4f {
                return textureSample(u_Texture, u_Sampler, v_TexCoord) * v_Color;
            }
        )";

        inline static constexpr std::string_view defaultTextFragmentShaderSource = R"(
            @group(1) @binding(0) var u_Texture: texture_2d<f32>;
            @group(1) @binding(1) var u_Sampler: sampler;

            @fragment
            fn fs_main(
                @location(0) v_TexCoord: vec2f,
                @location(1) v_Color: vec4f,
            ) -> @location(0) vec4f {
                let coverage = textureSample(u_Texture, u_Sampler, v_TexCoord).r;
                return vec4f(v_Color.rgb, v_Color.a * coverage);
            }
        )";

        inline static constexpr size_t MAX_VERTICES = 100'000;
        inline static constexpr size_t MAX_INDICES = 150'000;
    } // namespace detail
} // namespace p5

namespace p5
{
    Canvas::Canvas(GpuDevice& gpuDevice)
        : m_stateStack(),
          m_matrixStack(),
          m_renderer(Renderer::create(gpuDevice, detail::MAX_VERTICES, detail::MAX_INDICES)),
          m_defaultFillShader(loadShaderFromMemory(detail::defaultVertexShaderSource, detail::defaultFragmentShaderSource).value()),
          m_defaultTextShader(loadShaderFromMemory(detail::defaultVertexShaderSource, detail::defaultTextFragmentShaderSource).value()),
          m_defaultTexture(loadTexture(1, 1, std::array<uint8_t, 4> {255, 255, 255, 255}).value()),
          m_defaultFont(loadFont(std::span<const uint8_t> {DejaVuSans_ttf, DejaVuSans_ttf_len}).value()),
          m_geometrySubmitter(*m_renderer, m_defaultTexture, m_defaultFillShader, m_defaultTextShader),
          m_textRenderer(m_geometrySubmitter, m_matrixStack, m_defaultFont)
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

    void Canvas::strokeDashPattern(std::initializer_list<float> pattern)
    {
        DrawState& state = peekState();
        state.strokeDashPattern.assign(pattern.begin(), pattern.end());
    }

    void Canvas::strokeDashOffset(float offset)
    {
        DrawState& state = peekState();
        state.strokeDashOffset = offset;
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
        if (not(state.shader == shader)) {
            state.shaderUniforms.clear();
        }
        state.shader = shader;
    }

    void Canvas::noShader()
    {
        shader(Shader {});
    }

    void Canvas::setUniform(std::string_view name, float value)
    {
        peekState().setUniform(name, value);
    }

    void Canvas::setUniform(std::string_view name, const float2& value)
    {
        peekState().setUniform(name, value);
    }

    void Canvas::setUniform(std::string_view name, const float3& value)
    {
        peekState().setUniform(name, value);
    }

    void Canvas::setUniform(std::string_view name, const float4& value)
    {
        peekState().setUniform(name, value);
    }

    void Canvas::setUniform(std::string_view name, const matrix4x4& value)
    {
        peekState().setUniform(name, value);
    }

    float2 Canvas::applyTransform(const float2& point) const
    {
        return p5::transformPoint(m_matrixStack.peek(), point);
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

        m_geometrySubmitter.submitQuad(positions, texCoords, color, peekState());
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
            m_geometrySubmitter.submitFillMesh(ShapeMode::triangleFan, fillPositions, fillTexCoords, fillColors, state);
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

            m_geometrySubmitter.submitStroke(strokePositions, closeStroke, state.strokeColor, state);
        }
    }

    void Canvas::line(float x1, float y1, float x2, float y2)
    {
        const DrawState& state = peekState();
        if (not state.isStrokeEnabled)
            return;

        const float2 positions[2] = {applyTransform({x1, y1}), applyTransform({x2, y2})};
        m_geometrySubmitter.submitStroke(positions, false, state.strokeColor, state);
    }

    void Canvas::triangle(float x1, float y1, float x2, float y2, float x3, float y3)
    {
        const DrawState& state = peekState();
        ShapeBuilder builder;
        builder.beginShape(ShapeMode::triangles);

        const float minX = std::min({x1, x2, x3});
        const float maxX = std::max({x1, x2, x3});
        const float minY = std::min({y1, y2, y3});
        const float maxY = std::max({y1, y2, y3});
        const float width = maxX - minX;
        const float height = maxY - minY;

        const auto uvOf = [&](float x, float y) -> float2 {
            return {
                width > 1e-6f ? (x - minX) / width : 0.5f,
                height > 1e-6f ? (y - minY) / height : 0.5f,
            };
        };

        const float2 p1 = applyTransform({x1, y1});
        const float2 p2 = applyTransform({x2, y2});
        const float2 p3 = applyTransform({x3, y3});
        const float2 uv1 = uvOf(x1, y1);
        const float2 uv2 = uvOf(x2, y2);
        const float2 uv3 = uvOf(x3, y3);

        builder.vertex(p1.x, p1.y, uv1.x, uv1.y, state.fillColor, state.strokeColor);
        builder.vertex(p2.x, p2.y, uv2.x, uv2.y, state.fillColor, state.strokeColor);
        builder.vertex(p3.x, p3.y, uv3.x, uv3.y, state.fillColor, state.strokeColor);
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
            m_geometrySubmitter.submitFillMesh(ShapeMode::triangleFan, positions, texCoords, colors, state);
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

            m_geometrySubmitter.submitQuad(positions, texCoords, color, state);
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

        if (state.textureUVMode == TextureUVMode::pixel and state.texture.isValid()) {
            u /= static_cast<float>(state.texture.size.x);
            v /= static_cast<float>(state.texture.size.y);
        }

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
            m_geometrySubmitter.submitFillMesh(shape.mode, shape.positions, shape.texCoords, shape.fillColors, state);
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
                        m_geometrySubmitter.submitStroke(positions, texCoords, colors, false, state);
                    }
                    break;

                case ShapeMode::path:
                default:
                    m_geometrySubmitter.submitStroke(shape.positions, shape.texCoords, shape.strokeColors, close, state);
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

    void Canvas::texture(Texture texture)
    {
        peekState().texture = std::move(texture);
    }

    void Canvas::noTexture()
    {
        peekState().texture = Texture {};
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
            {u1, v1},
            {u2, v1},
            {u2, v2},
            {u1, v2},
        };

        m_geometrySubmitter.submitQuad(positions, texCoords, tintColor, state, texture);
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

    void Canvas::textLigatures(bool enabled)
    {
        DrawState& state = peekState();
        state.textLigatures = enabled;
    }

    void Canvas::text(std::string_view str, float x, float y, float maxWidth, float maxHeight)
    {
        m_textRenderer.text(peekState(), str, x, y, maxWidth, maxHeight);
    }

    void Canvas::text(std::u32string_view str, float x, float y, float maxWidth, float maxHeight)
    {
        m_textRenderer.text(peekState(), str, x, y, maxWidth, maxHeight);
    }

    float Canvas::textWidth(std::string_view str)
    {
        return m_textRenderer.textWidth(peekState(), str);
    }

    float Canvas::textWidth(std::u32string_view str)
    {
        return m_textRenderer.textWidth(peekState(), str);
    }

    rect2f Canvas::textBounds(std::string_view str, const TextBoundsOptions& options)
    {
        return m_textRenderer.textBounds(peekState(), str, options);
    }

    rect2f Canvas::textBounds(std::u32string_view str, const TextBoundsOptions& options)
    {
        return m_textRenderer.textBounds(peekState(), str, options);
    }

    std::vector<TextPoint> Canvas::textToPoints(std::string_view str, float x, float y, const TextToPointsOptions& options)
    {
        return m_textRenderer.textToPoints(peekState(), str, x, y, options);
    }

    std::vector<TextPoint> Canvas::textToPoints(std::u32string_view str, float x, float y, const TextToPointsOptions& options)
    {
        return m_textRenderer.textToPoints(peekState(), str, x, y, options);
    }
} // namespace p5
