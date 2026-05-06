#pragma once

#include <cmath>
#include <cstdint>
#include <memory>
#include <string_view>
#include <span>
#include <filesystem>

namespace p5
{
    float radians(float degrees);
    float degrees(float radians);
    float remap(float value, float fromLow, float fromHigh, float toLow, float toHigh);
    void randomSeed(uint64_t seed);
    float random(float max);
    float random(float min, float max);
} // namespace p5

namespace p5
{
    template <typename T>
    struct value2
    {
        T x, y;
    };

    typedef value2<float> float2;
    typedef value2<uint32_t> uint2;

    template <typename T> inline constexpr value2<T> operator-(value2<T> value) { return {-value.x, -value.y}; }

    template <typename T> inline constexpr value2<T> operator+(value2<T> lhs, value2<T> rhs) { return {lhs.x + rhs.x, lhs.y + rhs.y}; }
    template <typename T> inline constexpr value2<T> operator-(value2<T> lhs, value2<T> rhs) { return {lhs.x - rhs.x, lhs.y - rhs.y}; }
    template <typename T> inline constexpr value2<T> operator*(value2<T> lhs, value2<T> rhs) { return {lhs.x * rhs.x, lhs.y * rhs.y}; }
    template <typename T> inline constexpr value2<T> operator/(value2<T> lhs, value2<T> rhs) { return {lhs.x / rhs.x, lhs.y / rhs.y}; }

    template <typename T> inline constexpr value2<T> operator+(value2<T> lhs, T rhs) { return {lhs.x + rhs, lhs.y + rhs}; }
    template <typename T> inline constexpr value2<T> operator-(value2<T> lhs, T rhs) { return {lhs.x - rhs, lhs.y - rhs}; }
    template <typename T> inline constexpr value2<T> operator*(value2<T> lhs, T rhs) { return {lhs.x * rhs, lhs.y * rhs}; }
    template <typename T> inline constexpr value2<T> operator/(value2<T> lhs, T rhs) { return {lhs.x / rhs, lhs.y / rhs}; }

    template <typename T> inline constexpr value2<T> perp(value2<T> value) { return {-value.y, value.x}; }
    template <typename T> inline constexpr T dot(value2<T> a, value2<T> b) { return a.x * b.x + a.y * b.y; }
    template <typename T> inline constexpr T cross(value2<T> a, value2<T> b) { return a.x * b.y - a.y * b.x; }
    template <typename T> inline constexpr value2<T> lerp(value2<T> a, value2<T> b, T t) { return {std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t)}; }
    template <typename T> inline constexpr T lengthSquared(value2<T> value) { return value.x * value.x + value.y * value.y; }
    template <typename T> inline T length(value2<T> value) { return std::sqrt(lengthSquared(value)); }
    template <typename T> inline value2<T> normalized(value2<T> value)
    {
        const double len = static_cast<double>(length(value));
        if (len == 0.0) {
            return value2 {static_cast<T>(0), static_cast<T>(0)};
        }

        const double inv = 1.0 / len;
        const double x = static_cast<double>(value.x) * inv;
        const double y = static_cast<double>(value.y) * inv;

        return {static_cast<T>(x), static_cast<T>(y)};
    }

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

    struct matrix4x4
    {
        float m[16];

        static const matrix4x4 identity;
    };

    matrix4x4 combine(const matrix4x4& a, const matrix4x4& b);
    matrix4x4 translation(float x, float y);
    matrix4x4 scaling(float x, float y);
    matrix4x4 rotation(float angle);
    matrix4x4 ortho(float left, float right, float top, float bottom, float near, float far);
    matrix4x4 perspective(float fovY, float aspect, float near, float far);
    matrix4x4 lookAt(float2 eye, float2 center, float2 up);
    float2 transformPoint(const matrix4x4& matrix, float2 point);
} // namespace p5

namespace p5
{
    struct Sketch
    {
        virtual ~Sketch() = default;
        virtual void setup() = 0;
        virtual void draw() = 0;
        virtual void destroy() = 0;
        virtual void mousePressed(int x, int y) = 0;
    };

    extern std::unique_ptr<Sketch> createSketch();

    extern int mouseX;
    extern int mouseY;

    void info(std::string_view message);
    void debug(std::string_view message);
    void warning(std::string_view message);
    void error(std::string_view message);

    enum class StrokeCap {
        butt,
        square,
        round,
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
        baseline,
        bottom
    };

    enum class HorizontalTextAlign {
        left,
        center,
        right,
    };

    typedef size_t GlyphPageIndex;

    struct Glyph
    {
        float2 size;
        float2 bearing;
        rect2f uvRect;
        float2 advance;
        GlyphPageIndex pageIndex;
    };

    struct Font
    {
        virtual ~Font() = default;
        virtual uint32_t getGlyphPageTextureId(GlyphPageIndex index) = 0;
        virtual const Glyph* getFilledGlyph(char32_t codepoint, int textSize) = 0;
        virtual const Glyph* getStrokedGlyph(char32_t codepoint, int textSize, int strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit) = 0;
        virtual float getLineHeight(int textSize) const = 0;
        virtual float getKerning(char32_t leftCodepoint, char32_t rightCodepoint, int textSize) = 0;
    };

    struct TextMetrics
    {
        float width;
        float totalHeight;
        float ascender;
        float descender;
        uint32_t lineCount;
    };

    struct Shader
    {
        virtual ~Shader() = default;
        virtual void uploadMatrix(std::string_view name, const matrix4x4& matrix) = 0;
        virtual void uploadTexture(std::string_view name, uint32_t textureId) = 0;
        virtual void uploadInt1(std::string_view name, int x) = 0;
        virtual void uploadInt2(std::string_view name, int x, int y) = 0;
        virtual void uploadInt3(std::string_view name, int x, int y, int z) = 0;
        virtual void uploadInt4(std::string_view name, int x, int y, int z, int w) = 0;
        virtual void uploadFloat1(std::string_view name, float x) = 0;
        virtual void uploadFloat2(std::string_view name, float x, float y) = 0;
        virtual void uploadFloat3(std::string_view name, float x, float y, float z) = 0;
        virtual void uploadFloat4(std::string_view name, float x, float y, float z, float w) = 0;
        virtual uint32_t getRendererId() const = 0;
    };

    typedef uint32_t color_t;
    color_t color(int grey, int alpha = 255);
    color_t color(int red, int green, int blue, int alpha = 255);
    color_t lerp(color_t a, color_t b, float t);

    int red(color_t color);
    int green(color_t color);
    int blue(color_t color);
    int alpha(color_t color);
    int brightness(color_t color);

    size_t computeCircleSegmentCount(float angle, float radius);

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

    void background(int grey, int alpha = 255);
    void background(int red, int green, int blue, int alpha = 255);
    void background(color_t color);

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
    void strokePattern(std::span<const float> pattern);
    void strokePatternOffset(float offset);
    void miterLimit(float miterLimit);
    void roundJoinThreshold(float angleThreshold);

    void blendMode(BlendMode blendMode);
    void curveTightness(float tightness);
    void curveDetail(uint32_t detail);
    void bezierDetail(uint32_t detail);

    void beginShape();
    void endShape(bool close = true);
    void vertex(float x, float y);
    void vertex(float x, float y, float u, float v);
    void curveVertex(float x, float y);

    void shader(std::shared_ptr<Shader> shader);
    void noShader();

    std::unique_ptr<Font> loadFont(const std::filesystem::path& fontFilePath);
    TextMetrics measureText(std::string_view text);
    TextMetrics measureText(std::string_view text, Font* font, float textSize, float scale);
    void textAlign(HorizontalTextAlign horizontalAlign, VerticalTextAlign verticalAlign);

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
    void tint(color_t color);
    void noTint();
    void image(uint32_t textureId, float left, float top, float width, float height);
    void textFont(std::shared_ptr<Font> font);
    void noTextFont();
    void textSize(float size);
    void text(std::string_view text, float x, float y);
} // namespace p5
