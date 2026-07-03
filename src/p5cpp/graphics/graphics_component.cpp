#include <p5cpp/graphics/graphics_component.hpp>
#include <p5cpp/graphics/tess.hpp>

namespace p5cpp
{
    inline static constexpr size_t MAX_VERTICES = 65536;
    inline static constexpr size_t MAX_INDICES = 65536 * 3;

    GraphicsComponent::GraphicsComponent()
        : m_renderStateStack(),
          m_renderer(Renderer::create(MAX_VERTICES, MAX_INDICES))
    {
    }

    void GraphicsComponent::beginFrame()
    {
    }

    void GraphicsComponent::endFrame()
    {
    }

    void GraphicsComponent::pushCanvas(Framebuffer framebuffer)
    {
        m_renderer->flush();
        m_renderer->end();

        Framebuffer& current = m_framebufferStack.emplace_back(std::move(framebuffer));
        m_renderer->begin(current);
        m_renderStateStack.push();
    }

    void GraphicsComponent::popCanvas()
    {
        m_renderer->flush();
        m_renderer->end();

        m_framebufferStack.pop_back();
        if (not m_framebufferStack.empty()) {
            Framebuffer& current = m_framebufferStack.back();
            m_renderer->begin(current);
        }

        m_renderStateStack.pop();
    }

    void GraphicsComponent::pushState()
    {
        m_renderStateStack.push();
    }

    void GraphicsComponent::popState()
    {
        m_renderStateStack.pop();
    }

    void GraphicsComponent::pushMatrix()
    {
        RenderState& currentState = peekRenderState();
        currentState.metrics.push(currentState.metrics.peek());
    }

    void GraphicsComponent::popMatrix()
    {
        peekRenderState().metrics.pop();
    }

    void GraphicsComponent::resetMatrix()
    {
        peekRenderState().metrics.reset();
    }

    matrix4x4& GraphicsComponent::peekMatrix()
    {
        return peekRenderState().metrics.peek();
    }

    void GraphicsComponent::applyMatrix(const matrix4x4& matrix)
    {
        matrix4x4& currentMatrix = peekRenderState().metrics.peek();
        currentMatrix *= matrix;
    }

    void GraphicsComponent::setMatrix(const matrix4x4& matrix)
    {
        peekRenderState().metrics.peek() = matrix;
    }

    void GraphicsComponent::translate(float x, float y)
    {
        applyMatrix(matrix4x4::translation(x, y));
    }

    void GraphicsComponent::scale(float x, float y)
    {
        applyMatrix(matrix4x4::scaling(x, y));
    }

    void GraphicsComponent::rotate(float radians)
    {
        applyMatrix(matrix4x4::rotation(radians));
    }

    void GraphicsComponent::fill(color_t color)
    {
        RenderState& currentState = peekRenderState();
        currentState.fillColor = color;
        currentState.isFillDisabled = false;
    }

    void GraphicsComponent::noFill()
    {
        RenderState& currentState = peekRenderState();
        currentState.isFillDisabled = true;
    }

    void GraphicsComponent::stroke(color_t color)
    {
        RenderState& currentState = peekRenderState();
        currentState.strokeColor = color;
        currentState.isStrokeDisabled = false;
    }

    void GraphicsComponent::noStroke()
    {
        RenderState& currentState = peekRenderState();
        currentState.isStrokeDisabled = true;
    }

    void GraphicsComponent::strokeWeight(float strokeWeight)
    {
        peekRenderState().strokeWeight = strokeWeight;
    }

    void GraphicsComponent::strokeCap(StrokeCap strokeCap)
    {
        peekRenderState().strokeCap = strokeCap;
    }

    void GraphicsComponent::strokeJoin(StrokeJoin strokeJoin)
    {
        peekRenderState().strokeJoin = strokeJoin;
    }

    void GraphicsComponent::miterLimit(float miterLimit)
    {
        peekRenderState().miterLimit = miterLimit;
    }

    void GraphicsComponent::roundJoinThreshold(float roundJoinThreshold)
    {
        peekRenderState().roundJoinThreshold = roundJoinThreshold;
    }

    void GraphicsComponent::tint(color_t color)
    {
        RenderState& currentState = peekRenderState();
        currentState.tintColor = color;
    }

    void GraphicsComponent::noTint()
    {
        RenderState& currentState = peekRenderState();
        currentState.tintColor = rgba(255, 255, 255, 255);
    }

    void GraphicsComponent::bezierDetail(uint32_t detail)
    {
        RenderState& currentState = peekRenderState();
        currentState.bezierDetail = detail;
        currentState.invBezierDetail = 1.0f / static_cast<float>(detail);
    }

    void GraphicsComponent::curveTightness(float tightness)
    {
        peekRenderState().curveTightness = tightness;
    }

    void GraphicsComponent::curveDetail(uint32_t detail)
    {
        RenderState& currentState = peekRenderState();
        currentState.curveDetail = detail;
        currentState.invCurveDetail = 1.0f / static_cast<float>(detail);
    }

    void GraphicsComponent::textFont(const Font& font)
    {
        peekRenderState().font = font;
    }

    void GraphicsComponent::noTextFont()
    {
        peekRenderState().font = std::nullopt;
    }

    void GraphicsComponent::textSize(float size)
    {
        peekRenderState().textSize = size;
    }

    void GraphicsComponent::textLetterSpacing(float letterSpacing)
    {
        peekRenderState().textLetterSpacing = letterSpacing;
    }

    void GraphicsComponent::textLineSpacing(float lineSpacing)
    {
        peekRenderState().textLineSpacing = lineSpacing;
    }

    void GraphicsComponent::textAlign(TextAlign align)
    {
        peekRenderState().textAlign = align;
    }

    void GraphicsComponent::textWrap(TextWrap wrap)
    {
        peekRenderState().textWrap = wrap;
    }

    void GraphicsComponent::shader(const Shader& shader)
    {
        peekRenderState().shader = shader;
    }

    void GraphicsComponent::noShader()
    {
        peekRenderState().shader = std::nullopt;
    }

    void GraphicsComponent::blendMode(BlendMode blendMode)
    {
        peekRenderState().blendMode = blendMode;
    }

    RenderState& GraphicsComponent::peekRenderState()
    {
        return m_renderStateStack.peek();
    }

    void GraphicsComponent::background(color_t color)
    {
        const auto [w, h] = float2 {m_framebufferStack.back().getSize()};
        const std::array<float2, 4> positions = {
            float2 {0.0f, 0.0f},
            float2 {w, 0.0f},
            float2 {w, h},
            float2 {0.0f, h}
        };

        const std::array<float2, 4> texcoords = {
            float2 {0.0f, 0.0f},
            float2 {0.0f, 0.0f},
            float2 {0.0f, 0.0f},
            float2 {0.0f, 0.0f},
        };

        const std::array<color_t, 4> colors = {
            color,
            color,
            color,
            color
        };

        const RenderState& renderState = peekRenderState();
        DrawScope scope = m_renderer->getDrawScope();
        tesselate_quads(
            scope,
            PathPoints {
                .size = 4,
                .positions = positions,
                .texcoords = texcoords,
                .colors = colors,
            }
        );
        m_renderer->submit(scope, m_uniformCache, getCurrentShader(renderState), renderState.blendMode, m_whiteTexture);
    }

    void GraphicsComponent::rect(float left, float top, float width, float height)
    {
        const RenderState& renderState = peekRenderState();

        beginShape();
        vertex(left, top, 0.0f, 0.0f, renderState.fillColor, renderState.strokeColor);
        vertex(left + width, top, 0.0f, 0.0f, renderState.fillColor, renderState.strokeColor);
        vertex(left + width, top + height, 0.0f, 0.0f, renderState.fillColor, renderState.strokeColor);
        vertex(left, top + height, 1.0f, 0.0f, renderState.fillColor, renderState.strokeColor);
        endShape(ShapeType::quads, false);
    }

    void GraphicsComponent::rect(float left, float top, float width, float height, BorderRadius borderRadius)
    {
        const RenderState& renderState = peekRenderState();

        float maxRx = width * 0.5f;
        float maxRy = height * 0.5f;
        float topLeftX = std::min(borderRadius.topLeft.x, maxRx);
        float topLeftY = std::min(borderRadius.topLeft.y, maxRy);
        float topRightX = std::min(borderRadius.topRight.x, maxRx);
        float topRightY = std::min(borderRadius.topRight.y, maxRy);
        float bottomRightX = std::min(borderRadius.bottomRight.x, maxRx);
        float bottomRightY = std::min(borderRadius.bottomRight.y, maxRy);
        float bottomLeftX = std::min(borderRadius.bottomLeft.x, maxRx);
        float bottomLeftY = std::min(borderRadius.bottomLeft.y, maxRy);

        struct Corner
        {
            float cx, cy, rx, ry, startAngle;
        };

        const Corner corners[4] = {
            {left + width - bottomRightX, top + height - bottomRightY, bottomRightX, bottomRightY, 0.0f}, // unten rechts
            {left + bottomLeftX, top + height - bottomLeftY, bottomLeftX, bottomLeftY, HALF_PI},          // unten links
            {left + topLeftX, top + topLeftY, topLeftX, topLeftY, HALF_PI * 2},                           // oben links
            {left + width - topRightX, top + topRightY, topRightX, topRightY, HALF_PI * 3},               // oben rechts
        };

        // Emits all arc vertices for all 4 corners.
        // Each arc uses i < segs (open end) so consecutive corners share no duplicate junction vertex.
        // Zero-radius corners clamp to 1 segment, which emits exactly the corner point.
        const auto emitRim = [&] {
            for (const auto& c : corners) {
                const size_t segs = std::max(size_t(1), computeCircleSegmentCount(HALF_PI, std::max(c.rx, c.ry)));
                for (size_t i = 0; i < segs; ++i) {
                    const float angle = c.startAngle + HALF_PI * (float(i) / float(segs));
                    vertex(c.cx + std::cos(angle) * c.rx, c.cy + std::sin(angle) * c.ry, 0.0f, 0.0f, renderState.fillColor, renderState.strokeColor);
                }
            }
        };

        // Fill: triangleFan from the centre – rounded rect is always convex, so no libtess2 needed.
        if (not renderState.isFillDisabled) {
            beginShape();
            vertex(left + width * 0.5f, top + height * 0.5f, 0.0f, 0.0f, renderState.fillColor, renderState.strokeColor); // fan centre
            emitRim();
            vertex(corners[0].cx + corners[0].rx, corners[0].cy, 0.0f, 0.0f, renderState.fillColor, renderState.strokeColor); // re-emit first rim vertex to close the fan
            endShapeImpl(filled(ShapeType::triangleFan), std::nullopt, true, renderState);
        }

        // Stroke: lineLoop around the rim only (no internal fan edges).
        if (!renderState.isStrokeDisabled) {
            beginShape();
            emitRim();
            endShapeImpl(std::nullopt, stroked(ShapeType::lineLoop), true, renderState);
        }
    }

    void GraphicsComponent::ellipse(float centerX, float centerY, float width, float height)
    {
        const RenderState& renderState = peekRenderState();
    }

    void GraphicsComponent::endShapeImpl(const std::optional<ShapeDetails>& fill, const std::optional<ShapeDetails>& stroke, bool close, const RenderState& renderState)
    {
        DrawScope scope = m_renderer->getDrawScope();
        // TODO(Felix): Flush if needed

        if (fill.has_value()) {
            const PathPoints pathPoints = buildPathPoints(fill->colorChoice);

            switch (fill->shapeType) {
                case ShapeType::lines:
                case ShapeType::lineStrip:
                case ShapeType::lineLoop:
                case ShapeType::triangles: tesselate_triangles(scope, pathPoints); break;
                case ShapeType::triangleStrip: tesselate_triangle_strip(scope, pathPoints); break;
                case ShapeType::triangleFan: tesselate_triangle_fan(scope, pathPoints); break;
                case ShapeType::quads: tesselate_quads(scope, pathPoints); break;
                case ShapeType::quadStrip: tesselate_quad_strip(scope, pathPoints); break;
                case ShapeType::polygon: tesselate_polygon(scope, pathPoints); break;
            }

            m_renderer->submit(scope, m_uniformCache, getShader(renderState), renderState.blendMode, m_whiteTexture);
        }

        if (stroke.has_value()) {
            const PathPoints pathPoints = buildPathPoints(stroke->colorChoice);

            switch (stroke->shapeType) {
                case ShapeType::lines: stroke_lines(scope, pathPoints, renderState.strokeWeight, renderState.strokeCap, renderState.miterLimit, renderState.roundJoinThreshold); break;
                case ShapeType::lineStrip: stroke_line_strip(scope, pathPoints, renderState.strokeWeight, renderState.strokeCap, renderState.strokeJoin, renderState.miterLimit, renderState.roundJoinThreshold); break;
                case ShapeType::lineLoop: stroke_line_loop(scope, pathPoints, renderState.strokeWeight, renderState.strokeJoin, renderState.miterLimit, renderState.roundJoinThreshold); break;
                case ShapeType::triangles: stroke_triangles(scope, pathPoints, renderState.strokeWeight, renderState.strokeCap, renderState.strokeJoin, renderState.miterLimit, renderState.roundJoinThreshold); break;
                case ShapeType::triangleStrip: stroke_triangle_strip(scope, pathPoints, renderState.strokeWeight, renderState.strokeCap, renderState.strokeJoin, renderState.miterLimit, renderState.roundJoinThreshold); break;
                case ShapeType::triangleFan: stroke_triangle_fan(scope, pathPoints, renderState.strokeWeight, renderState.strokeCap, renderState.strokeJoin, renderState.miterLimit, renderState.roundJoinThreshold); break;
                case ShapeType::quads: stroke_quads(scope, pathPoints, renderState.strokeWeight, renderState.strokeCap, renderState.strokeJoin, renderState.miterLimit, renderState.roundJoinThreshold); break;
                case ShapeType::quadStrip: stroke_quad_strip(scope, pathPoints, renderState.strokeWeight, renderState.strokeCap, renderState.strokeJoin, renderState.miterLimit, renderState.roundJoinThreshold); break;
                case ShapeType::polygon: stroke_polygon(scope, pathPoints, renderState.strokeWeight, renderState.strokeCap, renderState.strokeJoin, renderState.miterLimit, renderState.roundJoinThreshold, close); break;
            }

            m_renderer->submit(scope, m_uniformCache, getShader(renderState), renderState.blendMode, m_whiteTexture);
        }
    }
} // namespace p5cpp
