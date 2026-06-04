#pragma once

#include <cmath>
#include <cstdint>
#include <memory>
#include <string_view>
#include <span>
#include <filesystem>
#include <array>
#include <vector>

namespace p5
{
    float radians(float degrees);
    float degrees(float radians);
    float remap(float value, float fromLow, float fromHigh, float toLow, float toHigh);
    void randomSeed(uint64_t seed);
    float random(float max);
    float random(float min, float max);

    float noise(float x);
    float noise(float x, float y);
    float noise(float x, float y, float z);
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
} // namespace p5

namespace p5
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
} // namespace p5

namespace p5
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

    extern int mouseX;
    extern int mouseY;

    extern int width;       // Current window width in logical points
    extern int height;      // Current window height in logical points
    extern int frameCount;  // Number of draw() calls completed
    extern float fps;       // Smoothed frames per second
    extern float deltaTime; // Seconds elapsed since the previous draw() call

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
        virtual const Glyph* getGlyph(char32_t codepoint, int textSize) = 0;
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

    typedef uint32_t color_t;

    struct Texture
    {
        virtual ~Texture() = default;
        virtual void update(std::span<const uint8_t> imageData) = 0;
        virtual void update(std::span<const color_t> pixelData) = 0;
        virtual uint32_t getRendererId() const = 0;
        virtual uint2 getSize() const = 0;
    };

    struct Framebuffer
    {
        virtual ~Framebuffer() = default;
        virtual uint32_t getTextureId() const = 0;
        virtual uint32_t getRendererId() const = 0;
        virtual uint2 getSize() const = 0;
        virtual uint2 getViewportSize() const = 0;
        virtual Texture* getColorTexture() = 0;
    };

    std::unique_ptr<Framebuffer> createCanvas(int width, int height);
    std::unique_ptr<Texture> createTexture(uint32_t width, uint32_t height);

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
    void endShape(ShapeType shapeType, bool close = true);
    void vertex(float x, float y);
    void vertex(float x, float y, float u, float v);
    void curveVertex(float x, float y);

    std::unique_ptr<Shader> loadShader(std::string_view vertexSource, std::string_view fragmentSource);

    void shader(std::shared_ptr<Shader> shader);
    void noShader();
    void setUniform(const std::string& name, const UniformVariable& variable);
    void setUniform(std::shared_ptr<Shader> shader, const std::string& name, const UniformVariable& variable);

    std::unique_ptr<Texture> loadTexture(const std::filesystem::path& imageFilePath);
    std::unique_ptr<Texture> loadTexture(uint32_t width, uint32_t height, std::span<const uint8_t> imageData);

    std::unique_ptr<Font> loadFont(const std::filesystem::path& fontFilePath);
    std::unique_ptr<Font> loadFont(std::span<const uint8_t> fontData);
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
    void tint(int grey, int alpha = 255);
    void tint(int red, int green, int blue, int alpha = 255);
    void tint(color_t color);
    void noTint();
    void image(uint32_t textureId, float left, float top, float width, float height);
    void textFont(std::shared_ptr<Font> font);
    void noTextFont();
    void textSize(float size);
    void text(std::string_view text, float x, float y);

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

    // ── Pixel manipulation ────────────────────────────────────────────────
    // Call loadPixels() to capture the current canvas into a Pixels object.
    // Modify pixel values freely via operator[] (index = y * pixels.width + x,
    // with (0,0) at top-left), then pass the object to updatePixels() to commit.
    struct Pixels
    {
        int width  = 0;
        int height = 0;

        color_t&       operator[](std::ptrdiff_t index)       { return m_data[static_cast<size_t>(index)]; }
        const color_t& operator[](std::ptrdiff_t index) const { return m_data[static_cast<size_t>(index)]; }

        color_t*       data()        { return m_data.data(); }
        const color_t* data()  const { return m_data.data(); }
        std::size_t    size()  const { return m_data.size(); }

        color_t*       begin()       { return m_data.data(); }
        color_t*       end()         { return m_data.data() + m_data.size(); }
        const color_t* begin() const { return m_data.data(); }
        const color_t* end()   const { return m_data.data() + m_data.size(); }

    private:
        explicit Pixels(int w, int h, std::vector<color_t> data)
            : width(w), height(h), m_data(std::move(data))
        {}

        std::vector<color_t> m_data;

        friend Pixels loadPixels();
    };

    Pixels loadPixels();
    void   updatePixels(Pixels& pixels);
} // namespace p5
