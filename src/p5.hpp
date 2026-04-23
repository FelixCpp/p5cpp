#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

namespace p5
{
    template <typename T>
    struct value2
    {
        T x, y;
    };

    typedef value2<float> float2;

    template <typename T>
    struct value3
    {
        T x, y, z;
    };

    typedef value3<float> float3;

    template <typename T>
    struct value4
    {
        T x, y, z, w;
    };

    typedef value4<float> float4;
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

    enum class StrokeAlign {
        inside,
        center,
        outside,
    };

    enum class BlendMode {
        none,
        alpha,
        additive,
        multiply,
    };

    typedef uint32_t color_t;
    color_t color(int grey, int alpha = 255);
    color_t color(int red, int green, int blue, int alpha = 255);

    int red(color_t color);
    int green(color_t color);
    int blue(color_t color);
    int alpha(color_t color);
    int brightness(color_t color);

    void pushState();
    void popState();

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
    void strokeAlign(StrokeAlign strokeAlign);
    void miterLimit(float miterLimit);

    void blendMode(BlendMode blendMode);

    void beginShape();
    void endShape();
    void vertex(float x, float y);
    void vertex(float x, float y, float u, float v);

    void rect(float left, float top, float width, float height);
    void rect(float left, float top, float width, float height, float cx, float cy);
    void rect(float left, float top, float width, float height, float topLeftX, float topLeftY, float topRightX, float topRightY, float bottomRightX, float bottomRightY, float bottomLeftX, float bottomLeftY);
    void square(float left, float top, float size);
    void ellipse(float centerX, float centerY, float width, float height);
    void circle(float centerX, float centerY, float size);
    void triangle(float x1, float y1, float x2, float y2, float x3, float y3);
} // namespace p5
