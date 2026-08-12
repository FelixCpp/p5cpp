#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/draw_state_stack.hpp>
#include <p5cpp/graphics/matrix_stack.hpp>
#include <p5cpp/graphics/framebuffer_stack.hpp>
#include <p5cpp/graphics/renderer.hpp>
#include <p5cpp/graphics/shape_builder.hpp>

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
        void curveTightness(float tightness);

        void tint(color_t color);
        void noTint();

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

        void beginShape(ShapeMode mode = ShapeMode::polygon);
        void vertex(float x, float y);
        void vertex(float x, float y, float u, float v);
        void bezierVertex(float controlX1, float controlY1, float controlX2, float controlY2, float x, float y);
        void quadraticVertex(float controlX, float controlY, float x, float y);
        void curveVertex(float x, float y);
        void endShape(bool close = false);

        void bezier(float x1, float y1, float controlX1, float controlY1, float controlX2, float controlY2, float x2, float y2);
        void curve(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4);

        void imageUVMode(TextureUVMode mode);
        void image(std::shared_ptr<Texture> texture, float left, float top, float width, float height);
        void image(std::shared_ptr<Texture> texture, float left, float top, float width, float height, float u1, float v1, float u2, float v2);

    private:
        std::shared_ptr<Shader> resolveActiveShader();
        std::shared_ptr<Texture> resolveActiveTexture(const std::shared_ptr<Texture>& texture = nullptr);

        void submitQuad(const std::span<const float2, 4>& positions, const std::span<const float2, 4>& texCoords, color_t color, const DrawState& state, const std::shared_ptr<Texture>& texture = nullptr);
        void submitStroke(const std::span<const float2>& positions, bool closed, color_t color, const DrawState& state);
        void submitStroke(const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const color_t>& colors, bool closed, const DrawState& state);
        void submitFillMesh(ShapeMode mode, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const color_t>& colors, const DrawState& state);
        void submitPoint(const float2& position, color_t color, const DrawState& state);
        void submitBuiltShape(const BuiltShape& shape, bool close);

        DrawStateStack m_stateStack;
        MatrixStack m_matrixStack;
        FramebufferStack m_framebufferStack;
        std::unique_ptr<Renderer> m_renderer;
        std::shared_ptr<Shader> m_defaultFillShader;
        std::shared_ptr<Texture> m_defaultTexture;
        ShapeBuilder m_shape;
    };
} // namespace p5
