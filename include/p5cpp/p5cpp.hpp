#pragma once

#include <p5cpp/application/sketch.hpp>
#include <p5cpp/application/window_event.hpp>
#include <p5cpp/application/window.hpp>
#include <p5cpp/application/logging.hpp>
#include <p5cpp/application/module.hpp>

#include <p5cpp/graphics/color.hpp>
#include <p5cpp/graphics/font.hpp>
#include <p5cpp/graphics/shader.hpp>
#include <p5cpp/graphics/framebuffer.hpp>
#include <p5cpp/graphics/shaping.hpp>
#include <p5cpp/graphics/texture.hpp>

#include <p5cpp/math/angle.hpp>
#include <p5cpp/math/constants.hpp>
#include <p5cpp/math/matrix4x4.hpp>
#include <p5cpp/math/noise.hpp>
#include <p5cpp/math/random.hpp>
#include <p5cpp/math/utility.hpp>
#include <p5cpp/math/value2.hpp>
#include <p5cpp/math/rectangle.hpp>

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>
#include <optional>

namespace p5cpp
{
    template <typename T>
    struct value4
    {
        T x, y, z, w;
    };

    typedef value4<float> float4;
} // namespace p5cpp

namespace p5cpp
{
    void setWindowSize(int width, int height);
    void setWindowTitle(std::string_view title);
    void setWindowResizable(bool resizable);

    int getMouseX();
    int getMouseY();
    int getPMouseX();
    int getPMouseY();

    int getLogicalWidth();
    int getLogicalHeight();
    int getPhysicalWidth();
    int getPhysicalHeight();

} // namespace p5cpp

namespace p5cpp
{
    void frameRate(int targetFps);
    void loop();
    void noLoop();
    bool isLooping();
    void quit();
    void quit(int code);
    void exitCode(int code);

    int getFrameCount();
    int getFrameRate();
    float getDeltaTime();
    float getGlobalTime();
} // namespace p5cpp

namespace p5cpp
{
    enum class StrokeCapStyle {
        butt,
        square,
        round,
    };

    struct StrokeCap
    {
        StrokeCapStyle start;
        StrokeCapStyle end;

        static const StrokeCap butt;
        static const StrokeCap square;
        static const StrokeCap round;
    };

    enum class StrokeJoin {
        miter,
        bevel,
        round
    };

    enum class BlendMode {
        none,
        alpha,
        additive,
        multiply,
    };

    enum class ArcMode {
        open,
        chord,
        pie,
    };

    enum class VerticalTextAlign {
        top,
        center,
        bottom,
        baseline,
    };

    enum class HorizontalTextAlign {
        left,
        center,
        right,
    };

    enum class TextWrap {
        none,
        word,
        character,
    };

    struct TextAlign
    {
        HorizontalTextAlign horizontal;
        VerticalTextAlign vertical;

        static const TextAlign topLeft;
        static const TextAlign topCenter;
        static const TextAlign topRight;

        static const TextAlign centerLeft;
        static const TextAlign center;
        static const TextAlign centerRight;

        static const TextAlign bottomLeft;
        static const TextAlign bottomCenter;
        static const TextAlign bottomRight;
    };

    struct LineLayout
    {
        size_t codepointsStart;
        size_t codepointsEnd;
        float width;
        float y;
    };

    struct GlyphQuad
    {
        float_rect vertexRect;
        float_rect uvRect;
        Texture* texture;
        size_t codepointIndex;
    };

    struct TextLayout
    {
        float totalWidth;
        float totalHeight;
        float ascender;
        float descender;
        std::vector<GlyphQuad> glyphs;
        std::vector<LineLayout> lines;
    };

    struct UniformVariable
    {
        enum class Type {
            float1,
            float2,
            float4,
            matrix4x4
        } type;

        union
        {
            float floatValue;
            float2 float2Value;
            float4 float4Value;
            matrix4x4 matrix4x4Value;
        };
    };

    UniformVariable uniform(float x);
    UniformVariable uniform(float x, float y);
    UniformVariable uniform(float x, float y, float z, float w);
    UniformVariable uniform(const matrix4x4& value);

    struct Pixels
    {
        int width;
        int height;
        std::vector<color_t> colors;
    };

    Pixels loadPixels();
    void updatePixels(const Pixels& pixels);

    size_t computeCircleSegmentCount(float angle, float radius);

    void pushCanvas(const Framebuffer& framebuffer);
    void popCanvas();

    void pushState();
    void popState();

    void pushMatrix();
    void popMatrix();
    void resetMatrix();
    matrix4x4& peekMatrix();
    void applyMatrix(const matrix4x4& matrix);
    void setMatrix(const matrix4x4& matrix);
    void translate(float x, float y);
    void scale(float x, float y);
    void rotate(float radians);

    void fill(int grey, int alpha = 255);
    void fill(int red, int green, int blue, int alpha = 255);
    void fill(color_t color);
    void noFill();

    void stroke(int grey, int alpha = 255);
    void stroke(int red, int green, int blue, int alpha = 255);
    void stroke(color_t color);
    void noStroke();

    void strokeWeight(float strokeWeight);
    void strokeCap(StrokeCap strokeCap);
    void strokeJoin(StrokeJoin strokeJoin);
    void miterLimit(float miterLimit);
    void roundJoinThreshold(float roundJoinThreshold);

    void tint(int grey, int alpha = 255);
    void tint(int red, int green, int blue, int alpha = 255);
    void tint(color_t color);
    void noTint();

    void bezierDetail(uint32_t detail);
    void curveTightness(float tightness);
    void curveDetail(uint32_t detail);

    void textFont(std::shared_ptr<Font> font);
    void noTextFont();
    void textSize(float size);
    void textLetterSpacing(float spacing);
    void textLineSpacing(float spacing);
    void textAlign(TextAlign textAlign);
    void textWrap(TextWrap textWrap);

    void shader(std::shared_ptr<ShaderImpl> shader);
    void noShader();
    void blendMode(BlendMode blendMode);

    void setUniform(const std::string& name, const UniformVariable& variable);
    void setUniform(std::shared_ptr<ShaderImpl> shader, const std::string& name, const UniformVariable& variable);

    void background(int grey, int alpha = 255);
    void background(int red, int green, int blue, int alpha = 255);
    void background(color_t color);

    void beginShape();
    void endShape(ShapeType shapeType, bool close = true);
    void vertex(float x, float y);
    void vertex(float x, float y, float u, float v);
    void curveVertex(float x, float y);

    void rect(float left, float top, float width, float height);
    void rect(float left, float top, float width, float height, float cx, float cy);
    void rect(float left, float top, float width, float height, float topLeftX, float topLeftY, float topRightX, float topRightY, float bottomRightX, float bottomRightY, float bottomLeftX, float bottomLeftY);
    void square(float left, float top, float size);
    void ellipse(float centerX, float centerY, float width, float height);
    void circle(float centerX, float centerY, float size);
    void point(float x, float y);
    void triangle(float x1, float y1, float x2, float y2, float x3, float y3);
    void line(float x1, float y1, float x2, float y2);
    void arc(float centerX, float centerY, float width, float height, float startAngle, float sweepAngle, ArcMode arcMode);
    void bezier(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4);
    void curve(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4);
    void image(const Texture* texture, float left, float top, float width, float height);
    void text(std::string_view text, float x, float y, std::optional<float> maxWidth = std::nullopt);

    TextLayout measureText(std::string_view text);
    TextLayout measureText(std::string_view text, Font* font, float textSize, float letterSpacing, float lineSpacing, TextAlign textAlign, TextWrap textWrap, std::optional<float> maxWidth);
} // namespace p5cpp
