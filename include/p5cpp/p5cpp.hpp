#pragma once

#include <p5cpp/application/sketch.hpp>
#include <p5cpp/application/window_event.hpp>
#include <p5cpp/application/window.hpp>
#include <p5cpp/application/logging.hpp>
#include <p5cpp/application/module.hpp>

#include <p5cpp/math/angle.hpp>
#include <p5cpp/math/constants.hpp>
#include <p5cpp/math/matrix4x4.hpp>
#include <p5cpp/math/noise.hpp>
#include <p5cpp/math/random.hpp>
#include <p5cpp/math/utility.hpp>
#include <p5cpp/math/value2.hpp>

#include <p5cpp/graphics/color.hpp>

#include <cstdint>
#include <memory>
#include <string_view>
#include <span>
#include <filesystem>
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

    template <typename T>
    struct rect2
    {
        T left, top, width, height;
    };

    typedef rect2<float> rect2f;
    typedef rect2<int32_t> rect2i;
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

    int getWidth();
    int getHeight();
    int getWindowWidth();
    int getWindowHeight();

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
    enum class ShapeType {
        lines,
        lineStrip,
        lineLoop,
        triangles,
        triangleStrip,
        triangleFan,
        quads,
        quadStrip,
        polygon,
    };

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

    struct Texture
    {
        virtual ~Texture() = default;
        virtual void update(std::span<const uint8_t> imageData) = 0;
        virtual void update(std::span<const color_t> pixelData) = 0;
        virtual uint32_t getRendererId() const = 0;
        virtual uint2 getSize() const = 0;
    };

    typedef size_t GlyphPageIndex;

    struct GlyphRegion
    {
        int2 size;
        rect2f uvRect;
    };

    struct Glyph
    {
        GlyphRegion region;
        int2 bearing;
        float advanceX;
        size_t glyphAtlasIndex;
    };

    struct FontMetrics
    {
        float ascender;
        float descender;
        float lineHeight;
    };

    struct Font
    {
        virtual ~Font() = default;
        virtual const Glyph* getGlyph(char32_t codepoint, int textSize) = 0;
        virtual const FontMetrics* getMetrics(int textSize) = 0;
        virtual float getKerning(char32_t leftCodepoint, char32_t rightCodepoint, int textSize) = 0;
        virtual Texture* getGlyphAtlasTexture(size_t glyphAtlasIndex) = 0;
    };

    std::unique_ptr<Font> loadFont(const std::filesystem::path& fontFilePath);
    std::unique_ptr<Font> loadFont(std::span<const uint8_t> fontData);
    Font* getCurrentFont();

    struct LineLayout
    {
        size_t codepointsStart;
        size_t codepointsEnd;
        float width;
        float y;
    };

    struct GlyphQuad
    {
        rect2f vertexRect;
        rect2f uvRect;
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

    std::unique_ptr<Texture> loadTexture(const std::filesystem::path& imageFilePath);
    std::unique_ptr<Texture> createTexture(uint32_t width, uint32_t height, std::span<const uint8_t> imageData);
    std::unique_ptr<Texture> createTexture(uint32_t width, uint32_t height);

    struct Framebuffer
    {
        virtual ~Framebuffer() = default;
        virtual uint32_t getTextureId() const = 0;
        virtual uint32_t getRendererId() const = 0;
        virtual uint2 getSize() const = 0;
        virtual uint2 getViewportSize() const = 0;
        virtual Texture* getColorTexture() = 0;
    };

    std::unique_ptr<Framebuffer> createFramebuffer(int width, int height);

    struct UniformVariable
    {
        enum class Type {
            float1,
            float2,
            float4,
            matrix4x4
        } type;

        union {
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

    struct Shader
    {
        virtual ~Shader() = default;
        virtual int getUniformLocation(std::string_view name) = 0;
        virtual uint32_t getRendererId() const = 0;
    };

    std::unique_ptr<Shader> loadShader(std::string_view vertexSource, std::string_view fragmentSource);

    struct Pixels
    {
        int width;
        int height;
        std::vector<color_t> colors;
    };

    Pixels loadPixels();
    void updatePixels(const Pixels& pixels);

    size_t computeCircleSegmentCount(float angle, float radius);

    void pushCanvas(std::shared_ptr<Framebuffer> canvas);
    void popCanvas();

    void pushState();
    void popState();

    void pushMatrix();
    void popMatrix();
    void resetMatrix();
    void applyMatrix(const matrix4x4& matrix);
    void setMatrix(const matrix4x4& matrix);
    matrix4x4& peekMatrix();
    void translate(float x, float y);
    void scale(float x, float y);
    void rotate(float angle);

    void noFill();
    void fill(int grey, int alpha = 255);
    void fill(int red, int green, int blue, int alpha = 255);
    void fill(color_t color);

    void noStroke();
    void stroke(int grey, int alpha = 255);
    void stroke(int red, int green, int blue, int alpha = 255);
    void stroke(color_t color);

    void strokeWeight(float strokeWeight);
    void strokeCap(StrokeCap strokeCap);
    void strokeJoin(StrokeJoin strokeJoin);
    void miterLimit(float miterLimit);
    void roundJoinThreshold(float angleThreshold);

    void blendMode(BlendMode blendMode);
    void curveTightness(float tightness);
    void curveDetail(uint32_t detail);
    void bezierDetail(uint32_t detail);

    void shader(std::shared_ptr<Shader> shader);
    void noShader();
    void setUniform(const std::string& name, const UniformVariable& variable);
    void setUniform(std::shared_ptr<Shader> shader, const std::string& name, const UniformVariable& variable);

    void textAlign(TextAlign textAlign);
    void textWrap(TextWrap textWrap);
    void textFont(std::shared_ptr<Font> font);
    void noTextFont();
    void textSize(float size);
    void textLetterSpacing(float spacing);
    void textLineSpacing(float spacing);

    void tint(int grey, int alpha = 255);
    void tint(int red, int green, int blue, int alpha = 255);
    void tint(color_t color);
    void noTint();

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
    void image(Texture* texture, float left, float top, float width, float height);
    void text(std::string_view text, float x, float y, std::optional<float> maxWidth = std::nullopt);

    TextLayout measureText(std::string_view text);
    TextLayout measureText(std::string_view text, Font* font, float textSize, float letterSpacing, float lineSpacing, TextAlign textAlign, TextWrap textWrap, std::optional<float> maxWidth);
} // namespace p5cpp
