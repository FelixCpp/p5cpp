#pragma once

#include <p5cpp/application/sketch.hpp>
#include <p5cpp/application/window_event.hpp>
#include <p5cpp/application/window.hpp>
#include <p5cpp/application/logging.hpp>
#include <p5cpp/application/module.hpp>

#include <p5cpp/graphics/blendmode.hpp>
#include <p5cpp/graphics/color.hpp>
#include <p5cpp/graphics/font.hpp>
#include <p5cpp/graphics/shader.hpp>
#include <p5cpp/graphics/framebuffer.hpp>
#include <p5cpp/graphics/shaping.hpp>
#include <p5cpp/graphics/texture.hpp>
#include <p5cpp/graphics/text.hpp>

#include <p5cpp/math/angle.hpp>
#include <p5cpp/math/constants.hpp>
#include <p5cpp/math/matrix4x4.hpp>
#include <p5cpp/math/noise.hpp>
#include <p5cpp/math/random.hpp>
#include <p5cpp/math/utility.hpp>
#include <p5cpp/math/value2.hpp>
#include <p5cpp/math/rectangle.hpp>

#include <cstdint>
#include <string_view>

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

    void textFont(Font font);
    void noTextFont();
    void textSize(float size);
    void textLetterSpacing(float spacing);
    void textLineSpacing(float spacing);
    void textAlign(TextAlign textAlign);
    void textWrap(TextWrap textWrap);

    void shader(const Shader& shader);
    void noShader();
    void blendMode(const BlendMode& blendMode);

    void setUniform(const std::string& name, const UniformVariable& variable);
    void setUniform(const Shader& shader, const std::string& name, const UniformVariable& variable);

    void background(int grey, int alpha = 255);
    void background(int red, int green, int blue, int alpha = 255);
    void background(color_t color);

    void beginShape();
    void endShape(ShapeType shapeType, bool close = true);
    void vertex(float x, float y);
    void vertex(float x, float y, float u, float v);
    void curveVertex(float x, float y);

    void rect(float left, float top, float width, float height);
    void rect(float left, float top, float width, float height, const BorderRadius& borderRadius);
    void square(float left, float top, float size);
    void ellipse(float centerX, float centerY, float radiusX, float radiusY);
    void circle(float centerX, float centerY, float radius);
    void point(float x, float y);
    void triangle(float x1, float y1, float x2, float y2, float x3, float y3);
    void line(float x1, float y1, float x2, float y2);
    void arc(float centerX, float centerY, float radiusX, float radiusY, float startAngle, float sweepAngle, ArcMode arcMode);
    void bezier(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4);
    void curve(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4);
    void image(const Texture& texture, float left, float top, float width, float height);
    void text(std::string_view text, float x, float y);
} // namespace p5cpp
