#include "p5cpp/application/logging.hpp"
#include <p5cpp/graphics/graphics_component.hpp>
#include <p5cpp/graphics/tess.hpp>

namespace p5cpp
{
    struct DefaultComputeCircleSegmentCount : public ComputeCircleSegmentCount
    {
        size_t operator()(float angle, float radius) const
        {
            const float error = 0.75f; // maximaler Fehler in Pixeln (tweakbar)

            if (radius <= 0.0f)
                return 0;

            // Clamp, um numerische Probleme zu vermeiden
            const float cosValue = 1.0f - (error / radius);
            const float clamped = std::clamp(cosValue, -1.0f, 1.0f);

            const float step = std::acos(clamped);

            if (step <= 0.0f)
                return 4;

            const size_t segments = static_cast<size_t>(std::ceil(std::abs(angle) / step));

            return std::max<size_t>(segments, 4);
        }
    };

    inline thread_local DefaultComputeCircleSegmentCount computeCircleSegmentCount;
} // namespace p5cpp

namespace p5cpp
{
    inline static constexpr size_t MAX_VERTICES = 65536;
    inline static constexpr size_t MAX_INDICES = 65536 * 3;

    GraphicsComponent::GraphicsComponent(uint32_t width, uint32_t height)
        : m_drawPointCount(0),
          m_drawPointCapacity(0),
          m_curveVertexCount(0),
          m_defaultFramebuffer(createFramebuffer(width, height)),
          m_renderStateStack(),
          m_renderer(NativeRenderer::create(MAX_VERTICES, MAX_INDICES))
    {
    }

    void GraphicsComponent::beginFrame()
    {
        pushCanvas(m_defaultFramebuffer);
    }

    void GraphicsComponent::endFrame()
    {
        popCanvas();
    }

    void GraphicsComponent::resizeDefaultCanvas(uint32_t width, uint32_t height)
    {
        m_defaultFramebuffer = createFramebuffer(width, height);
    }

    void GraphicsComponent::blitDefaultCanvasToScreen(uint32_t screenWidth, uint32_t screenHeight)
    {
        error("Not implemented: blitDefaultCanvasToScreen");
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

    void GraphicsComponent::setUniform(const std::string& name, const UniformVariable& variable)
    {
        const RenderState& renderState = peekRenderState();
        if (renderState.shader.has_value()) {
            m_uniformCache.setUniform(renderState.shader.value(), name, variable);
        }
    }

    void GraphicsComponent::setUniform(const Shader& shader, const std::string& name, const UniformVariable& variable)
    {
        m_uniformCache.setUniform(shader, name, variable);
    }

    RenderState& GraphicsComponent::peekRenderState()
    {
        return m_renderStateStack.peek();
    }

    void GraphicsComponent::background(color_t color)
    {
        error("Not implemented: background");
    }

    void GraphicsComponent::rect(float left, float top, float width, float height)
    {
        error("Not implemented: rect");
    }

    void GraphicsComponent::rect(float left, float top, float width, float height, BorderRadius borderRadius)
    {
        error("Not implemented: rect with border radius");
    }

    void GraphicsComponent::ellipse(float centerX, float centerY, float width, float height)
    {
        error("Not implemented: ellipse");
    }

    void GraphicsComponent::triangle(float x1, float y1, float x2, float y2, float x3, float y3)
    {
        error("Not implemented: triangle");
    }

    void GraphicsComponent::point(float centerX, float centerY)
    {
        error("Not implemented: point");
    }

    void GraphicsComponent::line(float x1, float y1, float x2, float y2)
    {
        error("Not implemented: line");
    }

    void GraphicsComponent::arc(float centerX, float centerY, float width, float height, float startAngle, float sweepAngle, ArcMode arcMode)
    {
        error("Not implemented: arc");
    }

    void GraphicsComponent::bezier(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
    {
        error("Not implemented: bezier");
    }

    void GraphicsComponent::curve(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
    {
        error("Not implemented: curve");
    }

    void GraphicsComponent::image(const Texture& texture, float left, float top, float width, float height)
    {
        error("Not implemented: image");
    }

    void GraphicsComponent::text(std::string_view text, float x, float y)
    {
        error("Not implemented: text");
    }

    void GraphicsComponent::beginShape()
    {
        error("Not implemented: beginShape");
    }

    void GraphicsComponent::endShape(ShapeType type, bool close)
    {
        error("Not implemented: endShape");
    }

    void GraphicsComponent::vertex(float x, float y, float u, float v)
    {
        if (m_drawPointCount >= m_drawPointCapacity) {
            const size_t newCapacity = std::max(m_drawPointCount * 2uz, 4uz);

            std::unique_ptr<float2[]> newPositions = std::make_unique<float2[]>(newCapacity);
            std::unique_ptr<float2[]> newTexCoords = std::make_unique<float2[]>(newCapacity);
            std::unique_ptr<color_t[]> newFillColors = std::make_unique<color_t[]>(newCapacity);
            std::unique_ptr<color_t[]> newStrokeColors = std::make_unique<color_t[]>(newCapacity);

            if (m_drawPointCount > 0) {
                std::copy(m_drawPointPositions.get(), m_drawPointPositions.get() + m_drawPointCount, newPositions.get());
                std::copy(m_drawPointTexCoords.get(), m_drawPointTexCoords.get() + m_drawPointCount, newTexCoords.get());
                std::copy(m_drawPointFillColors.get(), m_drawPointFillColors.get() + m_drawPointCount, newFillColors.get());
                std::copy(m_drawPointStrokeColors.get(), m_drawPointStrokeColors.get() + m_drawPointCount, newStrokeColors.get());
            }

            m_drawPointPositions = std::move(newPositions);
            m_drawPointTexCoords = std::move(newTexCoords);
            m_drawPointFillColors = std::move(newFillColors);
            m_drawPointStrokeColors = std::move(newStrokeColors);
            m_drawPointCapacity = newCapacity;
        }

        const RenderState& renderState = peekRenderState();
        const matrix4x4& matrix = renderState.metrics.peek();
        const float2 transformedPosition = matrix.transformPoint(x, y);

        m_drawPointPositions[m_drawPointCount] = transformedPosition;
        m_drawPointTexCoords[m_drawPointCount] = float2 {u, v};
        m_drawPointFillColors[m_drawPointCount] = renderState.fillColor;
        m_drawPointStrokeColors[m_drawPointCount] = renderState.strokeColor;
        ++m_drawPointCount;

        m_curveVertexCount = 0; // Reset curve vertex count when a regular vertex is added
    }

    void GraphicsComponent::curveVertex(float x, float y)
    {
        m_curveVertexPositions[m_curveVertexCount] = float2 {x, y};
        m_curveVertexCount++;

        if (m_curveVertexCount == 4) {
            const RenderState& renderState = peekRenderState();
            const float alpha = (1.0f - renderState.curveTightness) * 0.5f;

            auto [x1, y1] = m_curveVertexPositions[0];
            auto [x2, y2] = m_curveVertexPositions[1];
            auto [x3, y3] = m_curveVertexPositions[2];
            auto [x4, y4] = m_curveVertexPositions[3];

            for (size_t j = 0; j <= renderState.curveDetail; ++j) {
                float t = static_cast<float>(j) * renderState.invCurveDetail;
                float t2 = t * t;
                float t3 = t2 * t;

                float bx = alpha * ((-x1 + 3 * x2 - 3 * x3 + x4) * t3 + (2 * x1 - 5 * x2 + 4 * x3 - x4) * t2 + (-x1 + x3) * t) + x2;
                float by = alpha * ((-y1 + 3 * y2 - 3 * y3 + y4) * t3 + (2 * y1 - 5 * y2 + 4 * y3 - y4) * t2 + (-y1 + y3) * t) + y2;

                vertex(bx, by, 0.0f, 0.0f);
            }

            m_curveVertexCount = 0;
        }
    }

    void GraphicsComponent::endShapeImpl(ShapeType type, bool close, const RenderState& renderState)
    {
        error("Not implemented: endShapeImpl");
    }

    Shader GraphicsComponent::getShader(const RenderState& renderState)
    {
        if (renderState.shader.has_value()) {
            return renderState.shader.value();
        }

        return m_defaultShader;
    }
} // namespace p5cpp
