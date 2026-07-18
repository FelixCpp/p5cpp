#pragma once

#include <p5cpp/application/sketch.hpp>
#include <p5cpp/application/window_event.hpp>
#include <p5cpp/application/window.hpp>
#include <p5cpp/application/logging.hpp>
#include <p5cpp/application/module.hpp>

#include <p5cpp/audio/sound.hpp>
#include <p5cpp/audio/audio_stream.hpp>

#include <p5cpp/graphics/blendmode.hpp>
#include <p5cpp/graphics/color.hpp>
#include <p5cpp/graphics/filter.hpp>
#include <p5cpp/graphics/font.hpp>
#include <p5cpp/graphics/image.hpp>
#include <p5cpp/graphics/render_group.hpp>
#include <p5cpp/graphics/shader.hpp>
#include <p5cpp/graphics/framebuffer.hpp>
#include <p5cpp/graphics/shaping.hpp>
#include <p5cpp/graphics/texture.hpp>
#include <p5cpp/graphics/text.hpp>
#include <p5cpp/graphics/text_layout.hpp>

#include <p5cpp/math/angle.hpp>
#include <p5cpp/math/constants.hpp>
#include <p5cpp/math/matrix4x4.hpp>
#include <p5cpp/math/noise.hpp>
#include <p5cpp/math/random.hpp>
#include <p5cpp/math/utility.hpp>
#include <p5cpp/math/value2.hpp>
#include <p5cpp/math/rectangle.hpp>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <span>
#include <string_view>
#include <vector>

namespace p5cpp
{
    Sound loadSound(const std::filesystem::path& soundFilePath);
    Sound loadSound(std::span<const uint8_t> soundData);

    // Streams from disk while playing instead of decoding the whole file into
    // memory up front — use for long background music. Returns a regular Sound;
    // only clone()/playSoundMulti() are unsupported on streamed sounds.
    Sound loadMusic(const std::filesystem::path& musicFilePath);

    // Creates a playable sound from raw interleaved 32-bit float PCM samples.
    Sound createSound(std::span<const float> samples, uint32_t sampleRate, uint32_t channels);

    // Fully decodes an audio file into raw PCM samples for analysis/processing.
    AudioSamples loadAudioSamples(const std::filesystem::path& soundFilePath);
    AudioSamples loadAudioSamples(std::span<const uint8_t> soundData);

    // Procedural audio: the callback fills sample buffers on demand (on the
    // audio thread — see AudioStreamCallback in audio/audio_stream.hpp).
    AudioStream createAudioStream(uint32_t sampleRate, uint32_t channels, AudioStreamCallback callback);

    void playSound(const Sound& sound);
    void stopSound(const Sound& sound);
    void pauseSound(const Sound& sound);
    bool isPlaying(const Sound& sound);
    void seekSound(const Sound& sound, float seconds);

    // Fire-and-forget: plays an overlapping copy of the sound (polyphony); the
    // copy is cleaned up automatically when it finishes.
    void playSoundMulti(const Sound& sound);

    void masterVolume(float volume);
    float getMasterVolume();

    float getAudioAmplitude();
    std::span<const float> getAudioWaveform();
} // namespace p5cpp

namespace p5cpp
{
    void setWindowSize(int width, int height);
    void setWindowTitle(std::string_view title);
    void setWindowResizable(bool resizable);
    void setFullscreen(bool fullscreen);
    bool isFullscreen();

    int getMouseX();
    int getMouseY();
    int getPMouseX();
    int getPMouseY();

    int getLogicalWidth();
    int getLogicalHeight();
    int getPhysicalWidth();
    int getPhysicalHeight();

    // Continuous state: true for every frame the key/button is held down.
    bool isKeyDown(Key key);
    bool isMouseDown(MouseButton button);

    // Edge-triggered: true only for the frame in which the key/button went
    // down/up, respectively (does not re-trigger on OS key-repeat).
    bool isKeyPressed(Key key);
    bool isKeyReleased(Key key);
    bool isMousePressed(MouseButton button);
    bool isMouseReleased(MouseButton button);

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
    void restart();

    int getFrameCount();
    int getFrameRate();
    float getDeltaTime();
    float getGlobalTime();
} // namespace p5cpp

namespace p5cpp
{
    void pushCanvas(const Framebuffer& framebuffer);
    void popCanvas();
    uint2 getCanvasSize();
    std::vector<color_t> loadPixels();

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

    void filter(FilterType type, float amount);
    void effect(const Shader& shader);

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
    void text(std::string_view text, float x, float y, float maxWidth);

    // Computes how text(text, x, y[, maxWidth]) would lay the text out (line count,
    // width, height, bounding box, ...) without drawing anything.
    TextLayout textLayout(std::string_view text, float x, float y);
    TextLayout textLayout(std::string_view text, float x, float y, float maxWidth);

    // Builds geometry once by running buildFn (which can call fill()/stroke()/rect()/
    // ellipse()/beginShape()-vertex()-endShape()/image()/shader()+setUniform()/etc., or
    // drawRenderGroup() another group to compose) and returns a handle that replays the
    // already-tessellated geometry cheaply, as many times as you like, via
    // drawRenderGroup() — without re-tessellating on every call. buildFn runs with its
    // own isolated transform and fill/stroke state (starts at identity / sketch defaults,
    // independent of whatever is active at the call site); text()/background()/
    // pushCanvas()/popCanvas() are not supported inside buildFn.
    RenderGroup buildRenderGroup(const std::function<void()>& buildFn);

    // Replays a RenderGroup's recorded geometry at the current transform (pushMatrix()/
    // translate()/rotate()/scale() before calling this moves/rotates/scales the group).
    void drawRenderGroup(const RenderGroup& group);
    void drawRenderGroup(const RenderGroup& group, float x, float y); // sugar: translate(x,y) + drawRenderGroup(group)
} // namespace p5cpp
