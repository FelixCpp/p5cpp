#include <p5cpp/graphics/tessellators.hpp>
#include <p5cpp/graphics/graphics.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

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

            out vec4 v_Color;

            void main()
            {
                gl_Position = u_ProjectionMatrix * vec4(a_Position, 0.0, 1.0);
                v_Color = a_Color;
            }
        )";

        inline static constexpr std::string_view defaultFragmentShaderSource = R"(
            #version 410

            layout (location = 0) out vec4 o_FragColor;

            in vec4 v_Color;

            void main()
            {
                o_FragColor = v_Color;
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
    } // namespace detail
} // namespace p5

namespace p5
{
    Graphics::Graphics()
        : m_stateStack(),
          m_matrixStack(),
          m_renderer(Renderer::create(detail::MAX_VERTICES, detail::MAX_INDICES)),
          m_defaultFillShader(loadShaderFromMemory(detail::defaultVertexShaderSource, detail::defaultFragmentShaderSource))
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

    void Graphics::submitQuad(const std::span<const float2, 4>& positions, const std::span<const float2, 4>& texCoords, color_t color, const DrawState& state)
    {
        const float4 col = detail::toFloat4(color);
        const float4 colors[4] = {col, col, col, col};

        Renderer::Writer writer = m_renderer->write(4, 6, state.blendMode, resolveActiveShader());
        tesselate_quad(writer, positions, texCoords, colors);
        m_renderer->finish(writer);
    }

    void Graphics::submitFan(const std::span<const float2>& positions, const std::span<const float2>& texCoords, color_t color, const DrawState& state)
    {
        if (positions.size() < 3)
            return;

        const float4 col = detail::toFloat4(color);
        const std::vector<float4> colors(positions.size(), col);

        Renderer::Writer writer = m_renderer->write(positions.size(), (positions.size() - 2) * 3, state.blendMode, resolveActiveShader());
        tesselate_triangle_fan(writer, positions, texCoords, colors);
        m_renderer->finish(writer);
    }

    void Graphics::submitStroke(const std::span<const float2>& positions, bool closed, color_t color, const DrawState& state)
    {
        const PathTessellationBounds bounds = tesselate_path_bounds(positions.size(), closed, state.strokeCap, state.strokeJoin, state.strokeRoundJoinThreshold);
        if (bounds.maxVertexCount == 0 or bounds.maxIndexCount == 0)
            return;

        const float4 col = detail::toFloat4(color);
        const std::vector<float2> texCoords(positions.size(), float2 {0.0f, 0.0f});
        const std::vector<float4> colors(positions.size(), col);

        Renderer::Writer writer = m_renderer->write(bounds.maxVertexCount, bounds.maxIndexCount, state.blendMode, resolveActiveShader());
        tesselate_path(writer, positions, texCoords, colors, state.strokeWeight, state.strokeCap, state.strokeJoin, state.strokeMiterLimit, state.strokeRoundJoinThreshold, closed);
        m_renderer->finish(writer);
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
            Renderer::Writer writer = m_renderer->write(3, 3, state.blendMode, resolveActiveShader());
            tesselate_triangle(writer, positions, texCoords, colors);
            m_renderer->finish(writer);
        }

        if (state.isStrokeEnabled) {
            submitStroke(positions, true, state.strokeColor, state);
        }
    }

    void Graphics::point(float x, float y)
    {
        const DrawState& state = peekState();
        if (not state.isStrokeEnabled)
            return;

        const float radius = std::max(state.strokeWeight, 1.0f) * 0.5f;
        const bool round = state.strokeCap.start == StrokeCapStyle::round or state.strokeCap.end == StrokeCapStyle::round;

        if (round) {
            std::vector<float2> positions;
            std::vector<float2> texCoords;
            detail::buildEllipsePoints(x, y, radius, radius, positions, texCoords);
            submitFan(positions, texCoords, state.strokeColor, state);
        } else {
            const float2 positions[4] = {
                {x - radius, y - radius},
                {x + radius, y - radius},
                {x + radius, y + radius},
                {x - radius, y + radius},
            };

            const float2 texCoords[4] = {
                {0.0f, 0.0f},
                {1.0f, 0.0f},
                {1.0f, 1.0f},
                {0.0f, 1.0f},
            };

            submitQuad(positions, texCoords, state.strokeColor, state);
        }
    }

} // namespace p5
