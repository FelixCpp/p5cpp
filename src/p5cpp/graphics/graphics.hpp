#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/draw_state_stack.hpp>
#include <p5cpp/graphics/matrix_stack.hpp>
#include <p5cpp/graphics/framebuffer_stack.hpp>
#include <p5cpp/graphics/renderer.hpp>

namespace p5
{
    class Graphics
    {
    public:
        Graphics();

        void push();
        void pop();

        void pushFramebuffer(std::shared_ptr<Framebuffer> framebuffer);
        void popFramebuffer();
        std::shared_ptr<Framebuffer> peekFramebuffer() const;
        const uint2& getFramebufferSize() const;

        void pushState();
        void popState();
        DrawState& peekState();

        void pushMatrix();
        void popMatrix();
        matrix4x4& peekMatrix();
        void applyMatrix(const matrix4x4& matrix);
        void setMatrix(const matrix4x4& matrix);
        void translate(float x, float y);
        void scale(float x, float y);
        void rotate(float radians);

        void fill(color_t color);
        void noFill();

        void stroke(color_t color);
        void noStroke();

        void strokeWeight(float weight);
        void strokeCap(StrokeCap cap);
        void strokeJoin(StrokeJoin join);
        void strokeMiterLimit(float limit);
        void strokeRoundJoinThreshold(float threshold);

        void blendMode(const BlendMode& blendMode);

        void shader(std::shared_ptr<Shader> shader);
        void noShader();

        void setUniform(std::string_view name, float value);
        void setUniform(std::string_view name, const float2& value);
        void setUniform(std::string_view name, const float3& value);
        void setUniform(std::string_view name, const float4& value);
        void setUniform(std::string_view name, const matrix4x4& value);

        void background(color_t color);
        void rect(float left, float top, float width, float height);
        void square(float left, float top, float size);
        void ellipse(float centerX, float centerY, float radiusX, float radiusY);
        void circle(float centerX, float centerY, float radius);
        void line(float x1, float y1, float x2, float y2);
        void triangle(float x1, float y1, float x2, float y2, float x3, float y3);
        void point(float x, float y);

    private:
        std::shared_ptr<Shader> resolveActiveShader();

        void submitQuad(const std::span<const float2, 4>& positions, const std::span<const float2, 4>& texCoords, color_t color, const DrawState& state);
        void submitFan(const std::span<const float2>& positions, const std::span<const float2>& texCoords, color_t color, const DrawState& state);
        void submitStroke(const std::span<const float2>& positions, bool closed, color_t color, const DrawState& state);

        DrawStateStack m_stateStack;
        MatrixStack m_matrixStack;
        FramebufferStack m_framebufferStack;
        std::unique_ptr<Renderer> m_renderer;
        std::shared_ptr<Shader> m_defaultFillShader;
    };
} // namespace p5
