#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/draw_state_stack.hpp>
#include <p5cpp/graphics/matrix_stack.hpp>
#include <p5cpp/graphics/graphics_stack.hpp>
#include <p5cpp/graphics/renderer.hpp>
#include <p5cpp/graphics/shape_builder.hpp>

namespace p5
{
    class Canvas
    {
    public:
        Canvas();

        void push(bool extend);
        void pop();

        void pushGraphics(Graphics graphics, bool extend);
        void popGraphics();
        Graphics peekGraphics() const;
        uint2 getGraphicsSize() const;

        void flush();
        Pixels loadPixels();
        void updatePixels(const Pixels& pixels);

        void pushState(bool extend);
        void popState();
        DrawState& peekState();

        void pushMatrix(bool extend);
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

        void clip(float x, float y, float width, float height);
        void noClip();

        void shader(Shader shader);
        void noShader();

        void background(color_t color);
        void rect(float left, float top, float width, float height);
        void rect(float left, float top, float width, float height, const BorderRadius& borderRadius);
        void square(float left, float top, float size);
        void ellipse(float centerX, float centerY, float radiusX, float radiusY);
        void circle(float centerX, float centerY, float radius);
        void arc(float centerX, float centerY, float radiusX, float radiusY, float startAngle, float stopAngle, ArcMode mode = ArcMode::open);
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

        void textureUVMode(TextureUVMode mode);
        void textureFilter(TextureFilter filter);
        void textureWrap(TextureWrap wrap);
        void image(Texture texture, float left, float top, float width, float height);
        void image(Texture texture, float left, float top, float width, float height, float u1, float v1, float u2, float v2);

        void textFont(Font font);
        void noTextFont();
        void textSize(float pixels);
        void textAlign(TextAlignment alignment);
        void textWrap(TextWrap wrap);
        void textLeading(float pixels);
        void noTextLeading();
        void textLetterSpacing(float pixels);
        void text(std::string_view str, float x, float y, float maxWidth = 0.0f, float maxHeight = 0.0f);

        float textWidth(std::string_view str);
        rect2f textBounds(std::string_view str, float maxWidth = 0.0f);
        std::vector<TextPoint> textToPoints(std::string_view str, float x, float y, const TextToPointsOptions& options = {});

    private:
        Shader resolveActiveShader(const Shader& fallback);
        Texture resolveActiveTexture(const Texture& texture = {});
        float2 applyTransform(const float2& point) const;

        void submitQuad(const std::span<const float2, 4>& positions, const std::span<const float2, 4>& texCoords, color_t color, const DrawState& state, const Texture& texture = {});
        void submitStroke(const std::span<const float2>& positions, bool closed, color_t color, const DrawState& state);
        void submitStroke(const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const color_t>& colors, bool closed, const DrawState& state);
        void submitFillMesh(ShapeMode mode, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const color_t>& colors, const DrawState& state);
        void submitPoint(const float2& position, color_t color, const DrawState& state);
        void submitBuiltShape(const BuiltShape& shape, bool close);
        void submitTextMesh(const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const color_t>& colors, const Texture& atlasTexture, const DrawState& state);

        DrawStateStack m_stateStack;
        MatrixStack m_matrixStack;
        GraphicsStack m_graphicsStack;
        std::unique_ptr<Renderer> m_renderer;
        Shader m_defaultFillShader;
        Shader m_defaultTextShader;
        Texture m_defaultTexture;
        Font m_defaultFont;
        ShapeBuilder m_shape;
    };
} // namespace p5
