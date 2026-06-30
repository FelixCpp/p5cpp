#pragma once

#include <cmath>
#include <cstdint>
#include <memory>
#include <string_view>
#include <span>
#include <filesystem>
#include <array>
#include <vector>
#include <optional>

namespace p5cpp
{
    inline static constexpr float PI = 3.141592653589793238462643383279502f;
    inline static constexpr float TWO_PI = 6.283185307179586476925286766559005f;
    inline static constexpr float HALF_PI = 1.570796326794896619231321691639751f;
    inline static constexpr float QUARTER_PI = 0.785398163397448309615660845819875f;
    inline static constexpr float EULER = 2.718281828459045235360287471352662f;

    float radians(float degrees);
    float degrees(float radians);
    float remap(float value, float fromLow, float fromHigh, float toLow, float toHigh);
    void randomSeed(uint64_t seed);
    float randomFloat(float max);
    float randomFloat(float min, float max);
    int randomInt(int max);
    int randomInt(int min, int max);

    float noise(float x);
    float noise(float x, float y);
    float noise(float x, float y, float z);
} // namespace p5cpp

namespace p5cpp
{
    template <typename T>
    struct value2
    {
        T x, y;
    };

    typedef value2<float> float2;
    typedef value2<int32_t> int2;
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

    template <typename T> inline constexpr value2<T> operator+(T lhs, value2<T> rhs) { return {lhs + rhs.x, lhs + rhs.y}; }
    template <typename T> inline constexpr value2<T> operator-(T lhs, value2<T> rhs) { return {lhs - rhs.x, lhs - rhs.y}; }
    template <typename T> inline constexpr value2<T> operator*(T lhs, value2<T> rhs) { return {lhs * rhs.x, lhs * rhs.y}; }
    template <typename T> inline constexpr value2<T> operator/(T lhs, value2<T> rhs) { return {lhs / rhs.x, lhs / rhs.y}; }

    // clang-format off
    template <typename T> inline constexpr value2<T>& operator+=(value2<T>& lhs, value2<T> rhs) { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
    template <typename T> inline constexpr value2<T>& operator-=(value2<T>& lhs, value2<T> rhs) { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
    template <typename T> inline constexpr value2<T>& operator*=(value2<T>& lhs, value2<T> rhs) { lhs.x *= rhs.x; lhs.y *= rhs.y; return lhs; }
    template <typename T> inline constexpr value2<T>& operator/=(value2<T>& lhs, value2<T> rhs) { lhs.x /= rhs.x; lhs.y /= rhs.y; return lhs; }

    template <typename T> inline constexpr value2<T>& operator+=(value2<T>& lhs, T rhs) { lhs.x += rhs; lhs.y += rhs; return lhs; }
    template <typename T> inline constexpr value2<T>& operator-=(value2<T>& lhs, T rhs) { lhs.x -= rhs; lhs.y -= rhs; return lhs; }
    template <typename T> inline constexpr value2<T>& operator*=(value2<T>& lhs, T rhs) { lhs.x *= rhs; lhs.y *= rhs; return lhs; }
    template <typename T> inline constexpr value2<T>& operator/=(value2<T>& lhs, T rhs) { lhs.x /= rhs; lhs.y /= rhs; return lhs; }
    // clang-format on

    template <typename T> inline constexpr value2<T> perp(value2<T> value) { return {-value.y, value.x}; }
    template <typename T> inline constexpr T dot(value2<T> a, value2<T> b) { return a.x * b.x + a.y * b.y; }
    template <typename T> inline constexpr T cross(value2<T> a, value2<T> b) { return a.x * b.y - a.y * b.x; }
    template <typename T> inline constexpr value2<T> lerp(value2<T> a, value2<T> b, T t) { return {std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t)}; }
    template <typename T> inline constexpr T lengthSquared(value2<T> value) { return value.x * value.x + value.y * value.y; }
    template <typename T> inline T length(value2<T> value) { return std::sqrt(lengthSquared(value)); }
    template <typename T> inline value2<T> randomDirection()
    {
        const float angle = randomFloat(0.0f, 6.28318f);
        return value2<T> {std::cos(angle), std::sin(angle)};
    }
    template <typename T> inline value2<T> limit(value2<T> value, T maxLength)
    {
        const T lenSq = lengthSquared(value);
        if (lenSq > maxLength * maxLength) {
            const T len = length(value);
            const T scale = maxLength / len;
            return value * scale;
        }

        return value;
    }
    template <typename T> inline value2<T> fixedLength(value2<T> value, T newLength)
    {
        const double len = static_cast<double>(length(value));
        if (len == 0.0) {
            return value2 {static_cast<T>(0), static_cast<T>(0)};
        }

        const double scale = static_cast<double>(newLength) / len;
        const double x = static_cast<double>(value.x) * scale;
        const double y = static_cast<double>(value.y) * scale;

        return {static_cast<T>(x), static_cast<T>(y)};
    }
    template <typename T> inline value2<T> normalized(value2<T> value)
    {
        return fixedLength(value, static_cast<T>(1));
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
    typedef rect2<int32_t> rect2i;

    struct matrix4x4
    {
        std::array<float, 16> m;

        static const matrix4x4 identity;
    };

    matrix4x4 combine(const matrix4x4& a, const matrix4x4& b);
    matrix4x4 translation(float x, float y);
    matrix4x4 scaling(float x, float y);
    matrix4x4 rotation(float angle);
    matrix4x4 ortho(float left, float top, float right, float bottom, float near, float far);
    matrix4x4 perspective(float fovY, float aspect, float near, float far);
    matrix4x4 lookAt(float2 eye, float2 center, float2 up);
    float2 transformPoint(const matrix4x4& matrix, float2 point);
} // namespace p5cpp

namespace p5cpp
{
    // ── Mouse button ─────────────────────────────────────────────────────
    enum class MouseButton {
        left,
        middle,
        right,
    };

    // ── Keyboard key ─────────────────────────────────────────────────────
    // clang-format off
    enum class Key
    {
        unknown,

        // Printable characters
        space,
        apostrophe, comma, minus, period, slash,
        n0, n1, n2, n3, n4, n5, n6, n7, n8, n9,
        semicolon, equal,
        leftBracket, backslash, rightBracket, graveAccent,

        // Letters
        a, b, c, d, e, f, g, h, i, j, k, l, m,
        n, o, p, q, r, s, t, u, v, w, x, y, z,

        // Control & navigation
        escape, enter, tab, backspace,
        insert, del,
        right, left, down, up,
        pageUp, pageDown, home, end,

        // Function keys
        f1,  f2,  f3,  f4,  f5,  f6,
        f7,  f8,  f9,  f10, f11, f12,

        // Modifier keys
        leftShift,  leftCtrl,  leftAlt,  leftSuper,
        rightShift, rightCtrl, rightAlt, rightSuper,
    };
    // clang-format on

    // ── Modifier bitmasks ────────────────────────────────────────────────
    namespace KeyMod
    {
        inline constexpr int shift = 0x01;
        inline constexpr int ctrl = 0x02;
        inline constexpr int alt = 0x04;
        inline constexpr int super = 0x08;
    } // namespace KeyMod

    // ── Event type tag ───────────────────────────────────────────────────
    enum class EventType {
        close,
        mouseMove,
        mousePress,
        mouseRelease,
        mouseScroll,
        keyPress,
        keyRelease,
        keyRepeat,
        character,
        windowResize,
        framebufferResize,
    };

    // ── Unified window event (SDL-style: type tag + data union) ──────────
    struct WindowEvent
    {
        EventType type;

        struct MouseMoveData
        {
            int x, y;
        };

        struct MouseButtonData
        {
            MouseButton button;
            int x, y;
        };

        struct MouseScrollData
        {
            float dx, dy;
            int x, y;
        };

        struct KeyData
        {
            Key key;
            int mods;
        };

        struct CharData
        {
            char32_t codepoint;
        };

        struct ResizeData
        {
            int width, height;
        };

        union {
            MouseMoveData mouseMove;
            MouseButtonData mouseButton; // used for both mousePress and mouseRelease
            MouseScrollData mouseScroll;
            KeyData keyEvent; // used for keyPress, keyRelease and keyRepeat
            CharData charEvent;
            ResizeData windowResize;
            ResizeData framebufferResize;
        };
    };
} // namespace p5cpp

namespace p5cpp
{
    struct Sketch
    {
        virtual ~Sketch() = default;
        virtual void setup() = 0;
        virtual void draw() = 0;
        virtual void destroy() {}
        virtual void event(const WindowEvent&) {}
    };

    extern std::unique_ptr<Sketch> createSketch();

    int getMouseX();
    int getMouseY();
    int getPMouseX();
    int getPMouseY();

    int getWidth();
    int getHeight();

    int getFrameCount();
    int getFrameRate();
    float getDeltaTime();
    float getGlobalTime();

    void info(std::string_view message);
    void debug(std::string_view message);
    void warning(std::string_view message);
    void error(std::string_view message);

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

    typedef uint32_t color_t;

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

    color_t rgba(int grey, int alpha = 255);
    color_t rgba(int red, int green, int blue, int alpha = 255);
    color_t lighten(color_t color, float amount);
    color_t darken(color_t color, float amount);
    color_t lerp(color_t a, color_t b, float t);
    color_t withAlpha(color_t color, int alpha);

    int red(color_t color);
    int green(color_t color);
    int blue(color_t color);
    int alpha(color_t color);
    int brightness(color_t color);

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
    void image(std::shared_ptr<Texture> texture, float left, float top, float width, float height);
    void text(std::string_view text, float x, float y, std::optional<float> maxWidth = std::nullopt);

    TextLayout measureText(std::string_view text);
    TextLayout measureText(std::string_view text, Font* font, float textSize, float letterSpacing, float lineSpacing, TextAlign textAlign, TextWrap textWrap, std::optional<float> maxWidth);

    // ── Window management ─────────────────────────────────────────────────
    void setWindowSize(int width, int height);
    void setWindowTitle(std::string_view title);
    void setWindowResizable(bool resizable);
    int getWindowWidth();
    int getWindowHeight();

    // ── Timing & frame rate ───────────────────────────────────────────────
    void frameRate(float targetFps); // Limit to N fps; 0 = unlimited (default)
    void loop();
    void noLoop();
    bool isLooping();
    void quit();
    void quit(int exitCode);
    void exitCode(int code);
    float millis(); // Milliseconds since the application started
} // namespace p5cpp
