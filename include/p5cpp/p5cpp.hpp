#pragma once

#include <cstdint>
#include <variant>
#include <concepts>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <array>
#include <cmath>
#include <filesystem>
#include <format>

namespace p5
{
    struct matrix4x4
    {
        std::array<float, 16> m;

        constexpr bool operator==(const matrix4x4&) const = default;
    };

    constexpr matrix4x4 identityMatrix();
    constexpr matrix4x4 translationMatrix(float x, float y);
    constexpr matrix4x4 scalingMatrix(float x, float y);
    matrix4x4 rotationMatrix(float radians);
    constexpr matrix4x4 orthographicProjectionMatrix(float left, float top, float right, float bottom, float near, float far);
    constexpr matrix4x4 perspectiveProjectionMatrix(float fovY, float aspect, float near, float far);
    matrix4x4 lookAtMatrix(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ);
    constexpr matrix4x4 operator*(const matrix4x4& a, const matrix4x4& b);
} // namespace p5

namespace p5
{
    template <typename T> struct value2
    {
        T x, y;

        constexpr bool operator==(const value2&) const = default;
    };

    template <typename T> constexpr T lengthSquared(const value2<T>& v);
    template <typename T> constexpr T dot(const value2<T>& a, const value2<T>& b);
    template <typename T> T length(const value2<T>& v);
    template <typename T> value2<T> normalized(const value2<T>& v);
    template <typename T> value2<T> rotated(const value2<T>& v, float radians);
    template <typename T> value2<T> limited(const value2<T>& v, T maxLength);
    template <typename T> value2<T> fixedLength(const value2<T>& v, T length);
    template <typename T> constexpr value2<T> lerp(const value2<T>& a, const value2<T>& b, float t);
    template <typename T> constexpr value2<T> perpendicular(const value2<T>& v);
    template <typename T> constexpr T distanceSquared(const value2<T>& a, const value2<T>& b);
    template <typename T> T distance(const value2<T>& a, const value2<T>& b);

    template <typename T> constexpr value2<T> operator+(const value2<T>& a, const value2<T>& b);
    template <typename T> constexpr value2<T> operator-(const value2<T>& a, const value2<T>& b);
    template <typename T> constexpr value2<T> operator*(const value2<T>& a, const value2<T>& b);
    template <typename T> constexpr value2<T> operator/(const value2<T>& a, const value2<T>& b);

    template <typename T> constexpr value2<T> operator+(const value2<T>& a, T b);
    template <typename T> constexpr value2<T> operator-(const value2<T>& a, T b);
    template <typename T> constexpr value2<T> operator*(const value2<T>& a, T b);
    template <typename T> constexpr value2<T> operator/(const value2<T>& a, T b);

    template <typename T> constexpr value2<T> operator+(T a, const value2<T>& b);
    template <typename T> constexpr value2<T> operator-(T a, const value2<T>& b);
    template <typename T> constexpr value2<T> operator*(T a, const value2<T>& b);
    template <typename T> constexpr value2<T> operator/(T a, const value2<T>& b);
    template <typename T> constexpr value2<T> operator-(const value2<T>& v);

    typedef value2<float> float2;
    typedef value2<int32_t> int2;
    typedef value2<uint32_t> uint2;
} // namespace p5

namespace p5
{
    // Transforms a 2D point as if it were (x, y, 0, 1); ignores the matrix's z/w rows, since callers
    // only ever pass in affine 2D transforms (translate/scale/rotate) built by this library.
    constexpr float2 transformPoint(const matrix4x4& matrix, const float2& point);
} // namespace p5

namespace p5
{
    template <typename T> struct rect2
    {
        T x, y, width, height;

        constexpr bool operator==(const rect2&) const = default;
    };

    typedef rect2<float> rect2f;
    typedef rect2<int32_t> rect2i;
    typedef rect2<uint32_t> rect2u;
} // namespace p5

namespace p5
{
    template <typename T> struct value3
    {
        T x, y, z;

        constexpr bool operator==(const value3&) const = default;
    };

    typedef value3<float> float3;
    typedef value3<int32_t> int3;
    typedef value3<uint32_t> uint3;
} // namespace p5

namespace p5
{
    template <typename T> struct value4
    {
        T x, y, z, w;

        constexpr bool operator==(const value4&) const = default;
    };

    typedef value4<float> float4;
    typedef value4<int32_t> int4;
    typedef value4<uint32_t> uint4;
} // namespace p5

namespace p5
{
    typedef uint32_t color_t;
    constexpr color_t rgba(int32_t red, int32_t green, int32_t blue, int32_t alpha = 255);
    constexpr color_t rgba(int32_t grey, int32_t alpha = 255);

    constexpr uint8_t getRed(color_t color);
    constexpr uint8_t getGreen(color_t color);
    constexpr uint8_t getBlue(color_t color);
    constexpr uint8_t getAlpha(color_t color);
} // namespace p5

namespace p5
{
    constexpr float degrees(float radians);
    constexpr float radians(float degrees);

    constexpr float lerp(float a, float b, float t);
    constexpr float map(float value, float start1, float stop1, float start2, float stop2);
    constexpr float constrain(float value, float low, float high);

    void randomSeed(uint32_t seed);
    float random();
    float random(float max);
    float random(float min, float max);

    void noiseSeed(uint32_t seed);
    void noiseDetail(int octaves, float falloff = 0.5f);
    float noise(float x);
    float noise(float x, float y);
    float noise(float x, float y, float z);

    inline static constexpr float PI = 3.14159265358979323846f;
    inline static constexpr float TAU = 2.0f * PI;
    inline static constexpr float HALF_PI = 0.5f * PI;
} // namespace p5

namespace p5
{
    float easeLinear(float x);
    float easeInQuad(float x);
    float easeOutQuad(float x);
    float easeInOutQuad(float x);
    float easeInCubic(float x);
    float easeOutCubic(float x);
    float easeInOutCubic(float x);
    float easeInQuart(float x);
    float easeOutQuart(float x);
    float easeInOutQuart(float x);
    float easeInQuint(float x);
    float easeOutQuint(float x);
    float easeInOutQuint(float x);
    float easeInSine(float x);
    float easeOutSine(float x);
    float easeInOutSine(float x);
    float easeInExpo(float x);
    float easeOutExpo(float x);
    float easeInOutExpo(float x);
    float easeInCirc(float x);
    float easeOutCirc(float x);
    float easeInOutCirc(float x);
    float easeInBack(float x);
    float easeOutBack(float x);
    float easeInOutBack(float x);
    float easeInElastic(float x);
    float easeOutElastic(float x);
    float easeInOutElastic(float x);
    float easeInBounce(float x);
    float easeOutBounce(float x);
    float easeInOutBounce(float x);
} // namespace p5

namespace p5
{
    // clang-format off
    enum class Key
    {
        Unknown, Space, Apostrophe, Comma, Minus, Period, Slash,
        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
        Semicolon, Equal,
        A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        LeftBracket, Backslash, RightBracket, GraveAccent, World1, World2, Escape, Enter, Tab, Backspace, Insert, Delete,
        Right, Left, Down, Up,
        PageUp, PageDown,
        Home, End, CapsLock, ScrollLock, NumLock, PrintScreen, Pause,
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25,
        Keypad0, Keypad1, Keypad2, Keypad3, Keypad4, Keypad5, Keypad6, Keypad7, Keypad8, Keypad9,
        KeypadDecimal, KeypadDivide, KeypadMultiply, KeypadSubtract, KeypadAdd, KeypadEnter, KeypadEqual,
        LeftShift, LeftControl, LeftAlt, LeftSuper, RightShift,
        RightControl, RightAlt, RightSuper, Menu,
        Count,
    };
    // clang-format on

    // clang-format off
    enum class MouseButton
    {
        Left, Right, Middle, Button4, Button5, Button6, Button7, Button8,
        Count,
    };
    // clang-format on

    struct KeyMods
    {
        bool shift;
        bool control;
        bool alt;
        bool super;
        bool capsLock;
        bool numLock;
    };

    class WindowEvent
    {
    public:
        struct EventTypeTag
        {
        };

        struct Close : EventTypeTag
        {
        };

        struct WindowResize : EventTypeTag
        {
            uint32_t width;
            uint32_t height;
        };

        struct FramebufferResize : EventTypeTag
        {
            uint32_t width;
            uint32_t height;
        };

        struct FocusGained : EventTypeTag
        {
        };

        struct FocusLost : EventTypeTag
        {
        };

        struct WindowMove : EventTypeTag
        {
            int32_t x;
            int32_t y;
        };

        struct WindowMinimize : EventTypeTag
        {
        };

        struct WindowRestore : EventTypeTag
        {
        };

        struct WindowMaximize : EventTypeTag
        {
        };

        struct WindowUnmaximize : EventTypeTag
        {
        };

        struct WindowContentScaleChange : EventTypeTag
        {
            float xScale;
            float yScale;
        };

        struct KeyPress : EventTypeTag
        {
            Key key;
            int scancode;
            KeyMods mods;
            bool repeat;
        };

        struct KeyRelease : EventTypeTag
        {
            Key key;
            int scancode;
            KeyMods mods;
        };

        struct CharInput : EventTypeTag
        {
            uint32_t codepoint;
        };

        struct MouseButtonPress : EventTypeTag
        {
            MouseButton button;
            KeyMods mods;
            double x;
            double y;
        };

        struct MouseButtonRelease : EventTypeTag
        {
            MouseButton button;
            KeyMods mods;
            double x;
            double y;
        };

        struct MouseMove : EventTypeTag
        {
            double x;
            double y;
        };

        struct MouseScroll : EventTypeTag
        {
            double xOffset;
            double yOffset;
        };

        struct MouseEnter : EventTypeTag
        {
        };

        struct MouseLeave : EventTypeTag
        {
        };

        struct FileDrop : EventTypeTag
        {
            std::vector<std::string> paths;
        };

        using EventType = std::variant<
            Close, WindowResize, FramebufferResize, FocusGained, FocusLost,
            WindowMove, WindowMinimize, WindowRestore, WindowMaximize, WindowUnmaximize, WindowContentScaleChange,
            KeyPress, KeyRelease, CharInput,
            MouseButtonPress, MouseButtonRelease, MouseMove, MouseScroll, MouseEnter, MouseLeave,
            FileDrop>;

        constexpr WindowEvent(const EventType& eventType);

        template <std::derived_from<EventTypeTag> T> constexpr bool is() const;
        template <std::derived_from<EventTypeTag> T> constexpr const T& as() const;
        template <typename Visitor> constexpr decltype(auto) visit(Visitor&& visitor) const;

    private:
        EventType m_eventType;
    };
} // namespace p5

namespace p5
{
    struct Plugin;

    struct Sketch
    {
        virtual ~Sketch() = default;
        virtual void setup() {}
        virtual void event([[maybe_unused]] const WindowEvent& event) {}
        virtual void draw() {}
        virtual void destroy() {}
        virtual std::vector<std::unique_ptr<Plugin>> plugins();
    };

    extern std::unique_ptr<Sketch> createSketch();

    int getFrameCount();
    double getDeltaTime();
    double getGlobalTime();
} // namespace p5

namespace p5
{
    bool isKeyDown(Key key);
    bool isKeyPressed(Key key);
    bool isKeyReleased(Key key);

    bool isMouseButtonDown(MouseButton button);
    bool isMouseButtonPressed(MouseButton button);
    bool isMouseButtonReleased(MouseButton button);

    double getMouseX();
    double getMouseY();
    double getPreviousMouseX();
    double getPreviousMouseY();
    double getMouseDeltaX();
    double getMouseDeltaY();

    double getScrollX();
    double getScrollY();

    bool isCursorInWindow();

    std::span<const std::string> getDroppedFiles();
    std::span<const uint32_t> getTypedChars();
} // namespace p5

namespace p5
{
    void setWindowSize(uint32_t width, uint32_t height);
    void setWindowPosition(int32_t x, int32_t y);
    void setWindowTitle(std::string_view title);
    void setWindowResizable(bool resizable);
    void setWindowVisible(bool visible);

    uint2 getWindowSize();
    uint2 getWindowPhysicalSize();
    int2 getWindowPosition();
    std::string_view getWindowTitle();
    bool isWindowResizable();
    bool isWindowVisible();
} // namespace p5

namespace p5
{
    class Context
    {
    public:
        template <typename T> void provide(T* instance);
        template <typename T> void remove();
        template <typename T> T& require();
        template <typename T> T* get() const;
        template <typename T> bool has() const;

    private:
        std::unordered_map<std::type_index, void*> m_instances;
    };

} // namespace p5

namespace p5
{
    struct Plugin;

    class Next
    {
    public:
        explicit Next(std::span<const std::unique_ptr<Plugin>> chain, size_t index, Context* context, void (*step)(Plugin&, Context&, const Next&), const void* payload);
        void operator()() const;

        const void* getPayload() const;

    private:
        using Step = void (*)(Plugin&, Context&, const Next&);

        std::span<const std::unique_ptr<Plugin>> m_chain;
        size_t m_index;
        Context* context;
        Step m_step;
        const void* payload;
    };

    struct Plugin
    {
        virtual ~Plugin() = default;
        virtual void setup(Context& context, const Next& next);
        virtual void event(Context& context, const Next& next, const WindowEvent& event);
        virtual void draw(Context& context, const Next& next);
        virtual void destroy(Context& context, const Next& next);
    };

    inline std::vector<std::unique_ptr<Plugin>> Sketch::plugins()
    {
        return {};
    }
} // namespace p5

namespace p5::detail
{
    void logTrace(std::string_view message);
    void logInfo(std::string_view message);
    void logWarn(std::string_view message);
    void logError(std::string_view message);
} // namespace p5::detail

#define trace(...) p5::detail::logTrace(std::format(__VA_ARGS__))
#define info(...) p5::detail::logInfo(std::format(__VA_ARGS__))
#define warn(...) p5::detail::logWarn(std::format(__VA_ARGS__))
#define error(...) p5::detail::logError(std::format(__VA_ARGS__))

namespace p5
{
    enum class StrokeCapStyle
    {
        butt,
        square,
        triangle,
        round,
    };

    struct StrokeCap
    {
        StrokeCapStyle start;
        StrokeCapStyle end;

        static const StrokeCap butt;
        static const StrokeCap square;
        static const StrokeCap triangle;
        static const StrokeCap round;
    };

    enum class StrokeJoin
    {
        miter,
        bevel,
        round,
    };

    enum class ShapeMode
    {
        polygon,
        points,
        lines,
        path,
        triangles,
        triangleStrip,
        triangleFan,
        quads,
        quadStrip,
    };

    enum class ArcMode
    {
        open,
        chord,
        pie,
    };
} // namespace p5

namespace p5
{
    struct BlendMode
    {
        enum class Factor
        {
            zero,
            one,
            srcColor,
            oneMinusSrcColor,
            dstColor,
            oneMinusDstColor,
            srcAlpha,
            oneMinusSrcAlpha,
            dstAlpha,
            oneMinusDstAlpha,
        };

        enum class Equation
        {
            add,
            subtract,
            reverseSubtract,
            min,
            max,
        };

        constexpr BlendMode();
        constexpr BlendMode(Factor srcColorFactor, Factor dstColorFactor, Equation colorEquation, Factor srcAlphaFactor, Factor dstAlphaFactor, Equation alphaEquation);

        inline constexpr bool operator==(const BlendMode& other) const = default;
        inline constexpr bool operator!=(const BlendMode& other) const = default;

        static const BlendMode none;
        static const BlendMode alpha;
        static const BlendMode additive;
        static const BlendMode subtractive;
        static const BlendMode multiply;
        static const BlendMode screen;
        static const BlendMode darken;
        static const BlendMode lighten;
        static const BlendMode difference;
        static const BlendMode exclusion;

        Factor srcColorFactor;
        Factor dstColorFactor;
        Equation colorEquation;

        Factor srcAlphaFactor;
        Factor dstAlphaFactor;
        Equation alphaEquation;
    };
} // namespace p5

namespace p5
{
    struct Vertex
    {
        float2 position;
        float2 texCoord;
        float4 color;
    };
} // namespace p5

namespace p5
{
    using UniformValue = std::variant<float, float2, float3, float4, matrix4x4>;
} // namespace p5

namespace p5
{
    struct Shader
    {
        virtual ~Shader() = default;
        virtual uint32_t getShaderProgramId() const = 0;
        virtual int32_t getUniformLocation(std::string_view name) const = 0;

        std::unordered_map<std::string, UniformValue> uniforms;
    };

    std::unique_ptr<Shader> loadShaderFromMemory(std::string_view vertexShaderSource, std::string_view fragmentShaderSource);

    // Compiles a shader from just a fragment "effect" function, LÖVE2D-pixel-shader style — the library
    // supplies the vertex stage (position/UV/color pass-through) and the fragment stage's boilerplate,
    // so effectSource only needs to define:
    //
    //   vec4 effect(vec4 color, sampler2D image, vec2 texCoord, vec2 screenCoord) { ... }
    //
    // called once per fragment; its return value becomes the fragment's output color. Extra uniforms the
    // effect needs can be declared directly in effectSource (e.g. `uniform float u_Time;`) and set from
    // C++ via setUniform() as usual. For full control over the vertex stage too, use the two-argument
    // loadShaderFromMemory(vertexShaderSource, fragmentShaderSource) overload instead.
    std::unique_ptr<Shader> loadShaderFromMemory(std::string_view effectSource);

    void setUniform(Shader& shader, std::string_view name, float value);
    void setUniform(Shader& shader, std::string_view name, const float2& value);
    void setUniform(Shader& shader, std::string_view name, const float3& value);
    void setUniform(Shader& shader, std::string_view name, const float4& value);
    void setUniform(Shader& shader, std::string_view name, const matrix4x4& value);
} // namespace p5

namespace p5
{
    enum class TextureUVMode
    {
        normalized,
        pixel,
    };

    enum class TextureFilter
    {
        nearest,
        linear,
    };

    enum class TextureWrap
    {
        clampToEdge,
        repeat,
        mirroredRepeat,
    };
} // namespace p5

namespace p5
{
    enum class TexturePixelFormat
    {
        rgba8,
        r8,
    };

    struct Texture
    {
        virtual ~Texture() = default;
        virtual uint32_t getTextureId() const = 0;
        virtual const uint2& getSize() const = 0;
        virtual TexturePixelFormat getPixelFormat() const = 0;
        virtual void updateSubImage(uint32_t x, uint32_t y, uint32_t width, uint32_t height, std::span<const uint8_t> data) = 0;
        virtual std::vector<uint8_t> queryPixelData() const = 0;
    };

    std::unique_ptr<Texture> loadTextureFromMemory(uint32_t width, uint32_t height, std::span<const uint8_t> data, TexturePixelFormat format = TexturePixelFormat::rgba8);
    std::unique_ptr<Texture> loadTextureFromFile(const std::filesystem::path& filepath);
    bool saveTextureToFileAsPNG(const Texture& texture, const std::filesystem::path& filepath);
    bool saveTextureToFileAsJPEG(const Texture& texture, const std::filesystem::path& filepath, int quality = 90);
    bool saveTextureToFileAsBMP(const Texture& texture, const std::filesystem::path& filepath);
} // namespace p5

namespace p5
{
    struct Framebuffer
    {
        virtual ~Framebuffer() = default;
        virtual uint32_t getFramebufferId() const = 0;
        virtual std::shared_ptr<Texture> getColorTexture() const = 0;
        virtual const uint2& getSize() const = 0;
    };

    std::unique_ptr<Framebuffer> createFramebuffer(uint32_t width, uint32_t height);
} // namespace p5

namespace p5
{
    struct Pixels
    {
        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<color_t> data;
    };

    color_t getPixel(const Pixels& pixels, int32_t x, int32_t y);
    void setPixel(Pixels& pixels, int32_t x, int32_t y, color_t color);

    Pixels loadPixels(const Texture& texture);
    void updatePixels(Texture& texture, const Pixels& pixels);

    Pixels loadPixels(const Framebuffer& framebuffer);
    void updatePixels(Framebuffer& framebuffer, const Pixels& pixels);
} // namespace p5

namespace p5
{
    enum class TextAlignment
    {
        topLeft,
        topCenter,
        topRight,
        centerLeft,
        center,
        centerRight,
        bottomLeft,
        bottomCenter,
        bottomRight,
    };

    enum class TextWrap
    {
        none,
        character,
        word,
    };

    struct GlyphMetrics
    {
        rect2f uvRect;
        rect2f bounds;
        bool hasOutline;
    };

    struct ShapedGlyph
    {
        uint32_t glyphIndex;
        uint32_t cluster;
        float xAdvance;
        float yAdvance;
        float xOffset;
        float yOffset;
    };

    struct Font
    {
        virtual ~Font() = default;
        virtual std::vector<ShapedGlyph> shape(std::string_view utf8Text) const = 0;
        virtual const GlyphMetrics& getGlyphMetrics(uint32_t glyphIndex) = 0;
        virtual std::shared_ptr<Texture> getAtlasTexture() const = 0;
        virtual float getUnitsPerEm() const = 0;
        virtual float getAscent() const = 0;
        virtual float getDescent() const = 0;
        virtual float getLineGap() const = 0;
    };

    std::unique_ptr<Font> loadFontFromMemory(std::span<const uint8_t> data, uint32_t atlasWidth = 1024, uint32_t atlasHeight = 1024, uint32_t sdfSourceEmPixels = 128);
    std::unique_ptr<Font> loadFontFromFile(const std::filesystem::path& filepath, uint32_t atlasWidth = 1024, uint32_t atlasHeight = 1024, uint32_t sdfSourceEmPixels = 128);
} // namespace p5

namespace p5
{
    class CornerRadius : public float2
    {
    public:
        static constexpr CornerRadius circular(float radius);
        static constexpr CornerRadius elliptical(float radiusX, float radiusY);

        float radiusX;
        float radiusY;

    private:
        constexpr CornerRadius(float radiusX, float radiusY);
    };

    struct BorderRadius
    {
        CornerRadius topLeft;
        CornerRadius topRight;
        CornerRadius bottomRight;
        CornerRadius bottomLeft;

        static constexpr BorderRadius all(float radius);
        static constexpr BorderRadius symmetric(float horizontal, float vertical);
    };
} // namespace p5

namespace p5
{
    void pushFramebuffer(std::shared_ptr<Framebuffer> framebuffer);
    void popFramebuffer();
    const uint2& getFramebufferSize();
    float getWidth();
    float getHeight();

    void flush();

    Pixels loadPixels();
    void updatePixels(const Pixels& pixels);

    void push();
    void pop();

    void pushState();
    void popState();
    void pushMatrix();
    void popMatrix();
    void setMatrix(const matrix4x4& matrix);
    void applyMatrix(const matrix4x4& matrix);
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

    void blendMode(const BlendMode& blendMode);

    void clip(float x, float y, float width, float height);
    void noClip();

    void shader(std::shared_ptr<Shader> shader);
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
    void image(std::shared_ptr<Texture> texture, float left, float top, float width, float height);
    void image(std::shared_ptr<Texture> texture, float left, float top, float width, float height, float u1, float v1, float u2, float v2);

    void textFont(std::shared_ptr<Font> font);
    void noTextFont();
    void textSize(float pixels);
    void textAlign(TextAlignment alignment);
    void textWrap(TextWrap wrap);
    void textLeading(float pixels);
    void noTextLeading();
    void textLetterSpacing(float pixels);
    void text(std::string_view str, float x, float y, float maxWidth = 0.0f, float maxHeight = 0.0f);

    float textWidth(const Font& font, float size, std::string_view str, float letterSpacing = 0.0f);
    rect2f textBounds(const Font& font, float size, std::string_view str, TextWrap wrap = TextWrap::none, float maxWidth = 0.0f, float letterSpacing = 0.0f);
    float textWidth(std::string_view str);
    rect2f textBounds(std::string_view str, float maxWidth = 0.0f);

    template <std::invocable Func> void withFramebuffer(std::shared_ptr<Framebuffer> framebuffer, Func&& func);
    template <std::invocable Func> void withState(Func&& func);
    template <std::invocable Func> void withMatrix(Func&& func);
    template <std::invocable Func> void with(Func&& func);
    template <std::invocable Func> void withClip(float x, float y, float width, float height, Func&& func);
    template <std::invocable Func> void withShader(std::shared_ptr<Shader> shader, Func&& func);
    template <std::invocable Func> void withFont(std::shared_ptr<Font> font, Func&& func);
} // namespace p5

namespace p5
{
    inline constexpr matrix4x4 identityMatrix()
    {
        // clang-format off
        return matrix4x4 {
            .m = {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
            },
        };
        // clang-format on
    }

    inline constexpr matrix4x4 translationMatrix(float x, float y)
    {
        // clang-format off
        return matrix4x4 {
            .m = {
                1.0f, 0.0f, 0.0f, x,
                0.0f, 1.0f, 0.0f, y,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
            },
        };
        // clang-format on
    }

    inline constexpr matrix4x4 scalingMatrix(float x, float y)
    {
        // clang-format off
        return matrix4x4 {
            .m = {
                x,    0.0f, 0.0f, 0.0f,
                0.0f, y,    0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
            },
        };
        // clang-format on
    }

    inline matrix4x4 rotationMatrix(float radians)
    {
        const float cosTheta = std::cos(radians);
        const float sinTheta = std::sin(radians);

        // clang-format off
        return matrix4x4 {
            .m = {
                cosTheta, -sinTheta, 0.0f, 0.0f,
                sinTheta,  cosTheta, 0.0f, 0.0f,
                0.0f,      0.0f,     1.0f, 0.0f,
                0.0f,      0.0f,     0.0f, 1.0f,
            },
        };
        // clang-format on
    }

    inline constexpr matrix4x4 orthographicProjectionMatrix(float left, float top, float right, float bottom, float near, float far)
    {
        // clang-format off
        return matrix4x4 {
            .m = {
                2.0f / (right - left), 0.0f, 0.0f, -(right + left) / (right - left),
                0.0f, 2.0f / (top - bottom), 0.0f, -(top + bottom) / (top - bottom),
                0.0f, 0.0f, -2.0f / (far - near), -(far + near) / (far - near),
                0.0f, 0.0f, 0.0f, 1.0f,
            },
        };
        // clang-format on
    }

    inline constexpr matrix4x4 perspectiveProjectionMatrix(float fovY, float aspect, float near, float far)
    {
        const float f = 1.0f / std::tan(fovY / 2.0f);

        // clang-format off
        return matrix4x4 {
            .m = {
                f / aspect, 0.0f, 0.0f, 0.0f,
                0.0f, f, 0.0f, 0.0f,
                0.0f, 0.0f, (far + near) / (near - far), (2.0f * far * near) / (near - far),
                0.0f, 0.0f, -1.0f, 0.0f,
            },
        };
        // clang-format on
    }

    inline matrix4x4 lookAtMatrix(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ)
    {
        const float fX = centerX - eyeX;
        const float fY = centerY - eyeY;
        const float fZ = centerZ - eyeZ;

        const float fLength = std::sqrt(fX * fX + fY * fY + fZ * fZ);
        const float fNormX = fX / fLength;
        const float fNormY = fY / fLength;
        const float fNormZ = fZ / fLength;

        const float sX = fNormY * upZ - fNormZ * upY;
        const float sY = fNormZ * upX - fNormX * upZ;
        const float sZ = fNormX * upY - fNormY * upX;

        const float sLength = std::sqrt(sX * sX + sY * sY + sZ * sZ);
        const float sNormX = sX / sLength;
        const float sNormY = sY / sLength;
        const float sNormZ = sZ / sLength;

        const float uX = sNormY * fNormZ - sNormZ * fNormY;
        const float uY = sNormZ * fNormX - sNormX * fNormZ;
        const float uZ = sNormX * fNormY - sNormY * fNormX;

        // clang-format off
        return matrix4x4 {
            .m = {
                sNormX, uX, -fNormX, 0.0f,
                sNormY, uY, -fNormY, 0.0f,
                sNormZ, uZ, -fNormZ, 0.0f,
                -(sNormX * eyeX + sNormY * eyeY + sNormZ * eyeZ),
                -(uX * eyeX + uY * eyeY + uZ * eyeZ),
                (fNormX * eyeX + fNormY * eyeY + fNormZ * eyeZ),
                1.0f,
            },
        };
        // clang-format on
    }

    inline constexpr matrix4x4 operator*(const matrix4x4& a, const matrix4x4& b)
    {
        matrix4x4 result {};
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                result.m[row * 4 + col] =
                    a.m[row * 4 + 0] * b.m[0 * 4 + col] +
                    a.m[row * 4 + 1] * b.m[1 * 4 + col] +
                    a.m[row * 4 + 2] * b.m[2 * 4 + col] +
                    a.m[row * 4 + 3] * b.m[3 * 4 + col];
            }
        }
        return result;
    }

    inline constexpr float2 transformPoint(const matrix4x4& matrix, const float2& point)
    {
        return {
            matrix.m[0] * point.x + matrix.m[1] * point.y + matrix.m[3],
            matrix.m[4] * point.x + matrix.m[5] * point.y + matrix.m[7],
        };
    }
} // namespace p5

namespace p5
{
    template <typename T> inline constexpr T lengthSquared(const value2<T>& v) { return dot(v, v); }
    template <typename T> inline constexpr T dot(const value2<T>& a, const value2<T>& b) { return a.x * b.x + a.y * b.y; }
    template <typename T> inline T length(const value2<T>& v) { return std::sqrt(lengthSquared(v)); }
    template <typename T> inline value2<T> normalized(const value2<T>& v)
    {
        const T len = length(v);
        if (len == T {}) {
            return v; // zero vector has no direction; leave it as zero rather than producing NaN
        }
        return {v.x / len, v.y / len};
    }

    template <typename T> inline value2<T> rotated(const value2<T>& v, float radians)
    {
        const float cosTheta = std::cos(radians);
        const float sinTheta = std::sin(radians);
        return {static_cast<T>(v.x * cosTheta - v.y * sinTheta), static_cast<T>(v.x * sinTheta + v.y * cosTheta)};
    }

    template <typename T> inline value2<T> limited(const value2<T>& v, T maxLength)
    {
        const T len = lengthSquared(v);
        if (len > (maxLength * maxLength)) {
            return fixedLength(v, maxLength);
        }

        return v;
    }

    template <typename T> inline value2<T> fixedLength(const value2<T>& v, T newLength)
    {
        const T len = length(v);
        if (len == T {}) {
            return v; // zero vector has no direction; leave it as zero rather than producing NaN
        }
        return {v.x * newLength / len, v.y * newLength / len};
    }

    template <typename T> inline constexpr value2<T> lerp(const value2<T>& a, const value2<T>& b, float t) { return {std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t)}; }
    template <typename T> inline constexpr value2<T> perpendicular(const value2<T>& v) { return {-v.y, v.x}; }

    template <typename T> inline constexpr T distanceSquared(const value2<T>& a, const value2<T>& b) { return lengthSquared(b - a); }
    template <typename T> inline T distance(const value2<T>& a, const value2<T>& b) { return length(b - a); }

    template <typename T> inline constexpr value2<T> operator+(const value2<T>& a, const value2<T>& b) { return {a.x + b.x, a.y + b.y}; }
    template <typename T> inline constexpr value2<T> operator-(const value2<T>& a, const value2<T>& b) { return {a.x - b.x, a.y - b.y}; }
    template <typename T> inline constexpr value2<T> operator*(const value2<T>& a, const value2<T>& b) { return {a.x * b.x, a.y * b.y}; }
    template <typename T> inline constexpr value2<T> operator/(const value2<T>& a, const value2<T>& b) { return {a.x / b.x, a.y / b.y}; }
    template <typename T> inline constexpr value2<T> operator+(const value2<T>& a, T b) { return {a.x + b, a.y + b}; }
    template <typename T> inline constexpr value2<T> operator-(const value2<T>& a, T b) { return {a.x - b, a.y - b}; }
    template <typename T> inline constexpr value2<T> operator*(const value2<T>& a, T b) { return {a.x * b, a.y * b}; }
    template <typename T> inline constexpr value2<T> operator/(const value2<T>& a, T b) { return {a.x / b, a.y / b}; }
    template <typename T> inline constexpr value2<T> operator+(T a, const value2<T>& b) { return {a + b.x, a + b.y}; }
    template <typename T> inline constexpr value2<T> operator-(T a, const value2<T>& b) { return {a - b.x, a - b.y}; }
    template <typename T> inline constexpr value2<T> operator*(T a, const value2<T>& b) { return {a * b.x, a * b.y}; }
    template <typename T> inline constexpr value2<T> operator/(T a, const value2<T>& b) { return {a / b.x, a / b.y}; }
    template <typename T> inline constexpr value2<T> operator-(const value2<T>& v) { return {-v.x, -v.y}; }
} // namespace p5

namespace p5
{
    inline constexpr color_t rgba(int32_t red, int32_t green, int32_t blue, int32_t alpha) { return (static_cast<color_t>(red) << 24) | (static_cast<color_t>(green) << 16) | (static_cast<color_t>(blue) << 8) | static_cast<color_t>(alpha); }
    inline constexpr color_t rgba(int32_t grey, int32_t alpha) { return rgba(grey, grey, grey, alpha); }
    inline constexpr uint8_t getRed(color_t color) { return static_cast<uint8_t>((color >> 24) & 0xFF); }
    inline constexpr uint8_t getGreen(color_t color) { return static_cast<uint8_t>((color >> 16) & 0xFF); }
    inline constexpr uint8_t getBlue(color_t color) { return static_cast<uint8_t>((color >> 8) & 0xFF); }
    inline constexpr uint8_t getAlpha(color_t color) { return static_cast<uint8_t>(color & 0xFF); }
} // namespace p5

namespace p5
{
    // clang-format off
    constexpr float degrees(float radians) { return radians * (180.0f / PI); }
    constexpr float radians(float degrees) { return degrees * (PI / 180.0f); }
    constexpr float lerp(float a, float b, float t) { return std::lerp(a, b, t); }
    constexpr float map(float value, float start1, float stop1, float start2, float stop2) { return start2 + (stop2 - start2) * ((value - start1) / (stop1 - start1)); }
    constexpr float constrain(float value, float low, float high) { return (value < low) ? low : (value > high) ? high : value; }
    // clang-format on
} // namespace p5

namespace p5
{
    // clang-format off
    inline float easeLinear(float t) { return t; }
    inline float easeInQuad(float t) { return t * t; }
    inline float easeOutQuad(float t) { return t * (2.0f - t); }
    inline float easeInOutQuad(float t) { return (t < 0.5f) ? (2.0f * t * t) : (-1.0f + (4.0f - 2.0f * t) * t); }
    inline float easeInCubic(float t) { return t * t * t; }
    inline float easeOutCubic(float t) { return (--t) * t * t + 1.0f; }
    inline float easeInOutCubic(float t) { return (t < 0.5f) ? (4.0f * t * t * t) : ((t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f); }
    inline float easeInQuart(float t) { return t * t * t * t; }
    inline float easeOutQuart(float t) { return 1.0f - (--t) * t * t * t; }
    inline float easeInOutQuart(float t) { return (t < 0.5f) ? (8.0f * t * t * t * t) : (1.0f - 8.0f * (--t) * t * t * t); }
    inline float easeInQuint(float t) { return t * t * t * t * t; }
    inline float easeOutQuint(float t) { return 1.0f + (--t) * t * t * t * t; }
    inline float easeInOutQuint(float t) { return (t < 0.5f) ? (16.0f * t * t * t * t * t) : (1.0f + 16.0f * (--t) * t * t * t * t); }
    inline float easeInSine(float t) { return 1.0f - std::cos((t * PI) / 2.0f); }
    inline float easeOutSine(float t) { return std::sin((t * PI) / 2.0f); }
    inline float easeInOutSine(float t) { return -(std::cos(PI * t) - 1.0f) / 2.0f; }
    inline float easeInExpo(float t) { return (t == 0.0f) ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f)); }
    inline float easeOutExpo(float t) { return (t == 1.0f) ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t); }
    inline float easeInOutExpo(float t) { return (t == 0.0f) ? 0.0f : (t == 1.0f) ? 1.0f : (t < 0.5f) ? std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f; }
    inline float easeInCirc(float t) { return 1.0f - std::sqrt(1.0f - t * t); }
    inline float easeOutCirc(float t) { return std::sqrt(1.0f - (--t) * t); }
    inline float easeInOutCirc(float t) { return (t < 0.5f) ? (1.0f - std::sqrt(1.0f - 4.0f * t * t)) / 2.0f : (std::sqrt(1.0f - (--t) * (2.0f * t)) + 1.0f) / 2.0f; }
    inline float easeInBack(float t) { const float s = 1.70158f; return t * t * ((s + 1.0f) * t - s); }
    inline float easeOutBack(float t) { const float s = 1.70158f; return (--t) * t * ((s + 1.0f) * t + s) + 1.0f; }
    inline float easeInOutBack(float t) { const float s = 1.70158f * 1.525f; return (t < 0.5f) ? (t * t * ((s + 1.0f) * 2.0f * t - s)) : ((--t) * t * ((s + 1.0f) * 2.0f * t + s) + 1.0f); }
    inline float easeInElastic(float t) { return (t == 0.0f) ? 0.0f : (t == 1.0f) ? 1.0f : -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * ((2.0f * PI) / 3.0f)); }
    inline float easeOutElastic(float t) { return (t == 0.0f) ? 0.0f : (t == 1.0f) ? 1.0f : std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * ((2.0f * PI) / 3.0f)) + 1.0f; }
    inline float easeInOutElastic(float t) { return (t == 0.0f) ? 0.0f : (t == 1.0f) ? 1.0f : (t < 0.5f) ? -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * ((2.0f * PI) / 4.5f))) / 2.0f : (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * ((2.0f * PI) / 4.5f))) / 2.0f + 1.0f; }
    inline float easeInBounce(float t) { return 1.0f - easeOutBounce(1.0f - t); }
    inline float easeOutBounce(float t) { if (t < 1.0f / 2.75f) { return 7.5625f * t * t; } else if (t < 2.0f / 2.75f) { t -= 1.5f / 2.75f; return 7.5625f * t * t + 0.75f; } else if (t < 2.5f / 2.75f) { t -= 2.25f / 2.75f; return 7.5625f * t * t + 0.9375f; } else { t -= 2.625f / 2.75f; return 7.5625f * t * t + 0.984375f; } }
    inline float easeInOutBounce(float t) { return (t < 0.5f) ? (1.0f - easeOutBounce(1.0f - 2.0f * t)) / 2.0f : (1.0f + easeOutBounce(2.0f * t - 1.0f)) / 2.0f; }
    // clang-format on
} // namespace p5

namespace p5
{
    inline constexpr WindowEvent::WindowEvent(const EventType& eventType)
        : m_eventType(std::move(eventType))
    {
    }

    template <std::derived_from<WindowEvent::EventTypeTag> T>
    inline constexpr bool WindowEvent::is() const
    {
        return std::holds_alternative<T>(m_eventType);
    }

    template <std::derived_from<WindowEvent::EventTypeTag> T>
    inline constexpr const T& WindowEvent::as() const
    {
        return std::get<T>(m_eventType);
    }

    template <typename Visitor>
    inline constexpr decltype(auto) WindowEvent::visit(Visitor&& visitor) const
    {
        return std::visit(std::forward<Visitor>(visitor), m_eventType);
    }
} // namespace p5

namespace p5
{
    template <typename T>
    inline void Context::provide(T* instance)
    {
        m_instances.try_emplace(typeid(T), instance);
    }

    template <typename T>
    inline void Context::remove()
    {
        m_instances.erase(typeid(T));
    }

    template <typename T>
    inline T& Context::require()
    {
        return *get<T>();
    }

    template <typename T>
    inline T* Context::get() const
    {
        const auto it = m_instances.find(typeid(T));
        if (it == m_instances.end()) {
            error("Context::get() requested a type ({}) that was never provided", typeid(T).name());
            return nullptr;
        }
        return static_cast<T*>(it->second);
    }

    template <typename T>
    inline bool Context::has() const
    {
        return m_instances.contains(typeid(T));
    }
} // namespace p5

namespace p5
{
    inline constexpr StrokeCap StrokeCap::butt {.start = StrokeCapStyle::butt, .end = StrokeCapStyle::butt};
    inline constexpr StrokeCap StrokeCap::square {.start = StrokeCapStyle::square, .end = StrokeCapStyle::square};
    inline constexpr StrokeCap StrokeCap::triangle {.start = StrokeCapStyle::triangle, .end = StrokeCapStyle::triangle};
    inline constexpr StrokeCap StrokeCap::round {.start = StrokeCapStyle::round, .end = StrokeCapStyle::round};
} // namespace p5

namespace p5
{
    constexpr BlendMode::BlendMode()
        : srcColorFactor(Factor::one),
          dstColorFactor(Factor::zero),
          colorEquation(Equation::add),
          srcAlphaFactor(Factor::one),
          dstAlphaFactor(Factor::zero),
          alphaEquation(Equation::add)
    {
    }

    constexpr BlendMode::BlendMode(Factor srcColorFactor, Factor dstColorFactor, Equation colorEquation, Factor srcAlphaFactor, Factor dstAlphaFactor, Equation alphaEquation)
        : srcColorFactor(srcColorFactor),
          dstColorFactor(dstColorFactor),
          colorEquation(colorEquation),
          srcAlphaFactor(srcAlphaFactor),
          dstAlphaFactor(dstAlphaFactor),
          alphaEquation(alphaEquation)
    {
    }

    inline constexpr BlendMode BlendMode::none {Factor::one, Factor::zero, Equation::add, Factor::one, Factor::zero, Equation::add};
    inline constexpr BlendMode BlendMode::alpha {Factor::srcAlpha, Factor::oneMinusSrcAlpha, Equation::add, Factor::one, Factor::oneMinusSrcAlpha, Equation::add};
    inline constexpr BlendMode BlendMode::additive {Factor::srcAlpha, Factor::one, Equation::add, Factor::one, Factor::one, Equation::add};
    inline constexpr BlendMode BlendMode::subtractive {Factor::zero, Factor::oneMinusSrcColor, Equation::add, Factor::zero, Factor::oneMinusSrcColor, Equation::add};
    inline constexpr BlendMode BlendMode::multiply {Factor::dstColor, Factor::zero, Equation::add, Factor::dstAlpha, Factor::zero, Equation::add};
    inline constexpr BlendMode BlendMode::screen {Factor::one, Factor::oneMinusSrcColor, Equation::add, Factor::one, Factor::oneMinusSrcColor, Equation::add};
    inline constexpr BlendMode BlendMode::darken {Factor::one, Factor::one, Equation::min, Factor::one, Factor::one, Equation::min};
    inline constexpr BlendMode BlendMode::lighten {Factor::one, Factor::one, Equation::max, Factor::one, Factor::one, Equation::max};
    inline constexpr BlendMode BlendMode::difference {Factor::one, Factor::one, Equation::subtract, Factor::one, Factor::one, Equation::subtract};
    inline constexpr BlendMode BlendMode::exclusion {Factor::one, Factor::one, Equation::add, Factor::one, Factor::one, Equation::add};
} // namespace p5

namespace p5
{
    constexpr CornerRadius::CornerRadius(float radiusX, float radiusY)
        : radiusX(radiusX), radiusY(radiusY)
    {
    }

    constexpr CornerRadius CornerRadius::circular(float radius)
    {
        return CornerRadius(radius, radius);
    }

    constexpr CornerRadius CornerRadius::elliptical(float radiusX, float radiusY)
    {
        return CornerRadius(radiusX, radiusY);
    }
} // namespace p5

namespace p5
{
    constexpr BorderRadius BorderRadius::all(float radius)
    {
        return BorderRadius {CornerRadius::circular(radius), CornerRadius::circular(radius), CornerRadius::circular(radius), CornerRadius::circular(radius)};
    }

    constexpr BorderRadius BorderRadius::symmetric(float horizontal, float vertical)
    {
        const CornerRadius radius = CornerRadius::elliptical(horizontal, vertical);

        return BorderRadius {
            .topLeft = radius,
            .topRight = radius,
            .bottomRight = radius,
            .bottomLeft = radius,
        };
    }
} // namespace p5

namespace p5
{
    template <std::invocable Func>
    inline void withFramebuffer(std::shared_ptr<Framebuffer> framebuffer, Func&& func)
    {
        try {
            pushFramebuffer(std::move(framebuffer));
            func();
            popFramebuffer();
        } catch (...) {
            popFramebuffer();
            throw;
        }
    }

    template <std::invocable Func>
    inline void withState(Func&& func)
    {
        try {
            pushState();
            func();
            popState();
        } catch (...) {
            popState();
            throw;
        }
    }

    template <std::invocable Func>
    inline void withMatrix(Func&& func)
    {
        try {
            pushMatrix();
            func();
            popMatrix();
        } catch (...) {
            popMatrix();
            throw;
        }
    }

    template <std::invocable Func>
    inline void with(Func&& func)
    {
        try {
            push();
            func();
            pop();
        } catch (...) {
            pop();
            throw;
        }
    }

    template <std::invocable Func>
    inline void withClip(float x, float y, float width, float height, Func&& func)
    {
        try {
            clip(x, y, width, height);
            func();
            noClip();
        } catch (...) {
            noClip();
            throw;
        }
    }

    template <std::invocable Func>
    inline void withShader(std::shared_ptr<Shader> shader, Func&& func)
    {
        try {
            p5::shader(std::move(shader));
            func();
            noShader();
        } catch (...) {
            noShader();
            throw;
        }
    }

    template <std::invocable Func>
    inline void withFont(std::shared_ptr<Font> font, Func&& func)
    {
        try {
            textFont(std::move(font));
            func();
            noTextFont();
        } catch (...) {
            noTextFont();
            throw;
        }
    }
} // namespace p5
