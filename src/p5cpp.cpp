#include <p5cpp/p5cpp.hpp>

#include <p5cpp/application/engine.hpp>
#include <p5cpp/application/app_context.hpp>
#include <p5cpp/application/frame_module.hpp>
#include <p5cpp/application/window_module.hpp>
#include <p5cpp/application/input_module.hpp>
#include <p5cpp/application/sketch_module.hpp>

#include <p5cpp/graphics/graphics_module.hpp>

#include <cassert>

namespace p5cpp
{
    std::unique_ptr<Engine> engine;

    inline static AppContext& getAppContext()
    {
        return engine->getContext();
    }

    inline Window& getWindow()
    {
        return getAppContext().require<Window>();
    }

    inline static InputComponent& getInputComponent()
    {
        return getAppContext().require<InputComponent>();
    }

    inline static FrameComponent& getFrameComponent()
    {
        return getAppContext().require<FrameComponent>();
    }

    inline static GraphicsComponent& getGraphicsComponent()
    {
        return getAppContext().require<GraphicsComponent>();
    }
} // namespace p5cpp

namespace p5cpp
{
    void setWindowSize(int width, int height) { getWindow().setSize(width, height); }
    void setWindowTitle(std::string_view title) { getWindow().setTitle(title); }
    void setWindowResizable(bool resizable) { getWindow().setResizable(resizable); }

    int getMouseX() { return getInputComponent().getMouseX(); }
    int getMouseY() { return getInputComponent().getMouseY(); }
    int getPMouseX() { return getInputComponent().getPMouseX(); }
    int getPMouseY() { return getInputComponent().getPMouseY(); }

    int getLogicalWidth() { return getInputComponent().getLogicalWidth(); }
    int getLogicalHeight() { return getInputComponent().getLogicalHeight(); }
    int getPhysicalWidth() { return getInputComponent().getPhysicalWidth(); }
    int getPhysicalHeight() { return getInputComponent().getPhysicalHeight(); }
} // namespace p5cpp

namespace p5cpp
{
    void frameRate(int targetFps) { getFrameComponent().frameRate(targetFps); }
    void loop() { getFrameComponent().loop(); }
    void noLoop() { getFrameComponent().noLoop(); }
    bool isLooping() { return getFrameComponent().isLooping(); }
    void quit() { getFrameComponent().quit(); }
    void quit(int code) { getFrameComponent().quit(code); }
    void exitCode(int code) { getFrameComponent().exitCode(code); }

    int getFrameCount() { return getFrameComponent().getFrameCount(); }
    int getFrameRate() { return getFrameComponent().getFrameRate(); }
    float getDeltaTime() { return getFrameComponent().getDeltaTime(); }
    float getGlobalTime() { return getFrameComponent().getGlobalTime(); }
} // namespace p5cpp

namespace p5cpp
{
    void pushCanvas(const Framebuffer& framebuffer) { getGraphicsComponent().pushCanvas(framebuffer); }
    void popCanvas() { getGraphicsComponent().popCanvas(); }

    void pushState() { getGraphicsComponent().pushState(); }
    void popState() { getGraphicsComponent().popState(); }

    void pushMatrix() { getGraphicsComponent().pushMatrix(); }
    void popMatrix() { getGraphicsComponent().popMatrix(); }
    void resetMatrix() { getGraphicsComponent().resetMatrix(); }
    matrix4x4& peekMatrix() { return getGraphicsComponent().peekMatrix(); }
    void applyMatrix(const matrix4x4& matrix) { getGraphicsComponent().applyMatrix(matrix); }
    void setMatrix(const matrix4x4& matrix) { getGraphicsComponent().setMatrix(matrix); }
    void translate(float x, float y) { getGraphicsComponent().translate(x, y); }
    void scale(float x, float y) { getGraphicsComponent().scale(x, y); }
    void rotate(float radians) { getGraphicsComponent().rotate(radians); }

    void fill(int grey, int alpha) { fill(grey, grey, grey, alpha); }
    void fill(int red, int green, int blue, int alpha) { fill(rgba(red, green, blue, alpha)); }
    void fill(color_t color) { getGraphicsComponent().fill(color); }
    void noFill() { getGraphicsComponent().noFill(); }

    void stroke(int grey, int alpha) { stroke(grey, grey, grey, alpha); }
    void stroke(int red, int green, int blue, int alpha) { stroke(rgba(red, green, blue, alpha)); }
    void stroke(color_t color) { getGraphicsComponent().fill(color); }
    void noStroke() { getGraphicsComponent().noStroke(); }

    void strokeWeight(float strokeWeight) { getGraphicsComponent().strokeWeight(strokeWeight); }
    void strokeCap(StrokeCap strokeCap) { getGraphicsComponent().strokeCap(strokeCap); }
    void strokeJoin(StrokeJoin strokeJoin) { getGraphicsComponent().strokeJoin(strokeJoin); }
    void miterLimit(float miterLimit) { getGraphicsComponent().miterLimit(miterLimit); }
    void roundJoinThreshold(float roundJoinThreshold) { getGraphicsComponent().roundJoinThreshold(roundJoinThreshold); }

    void tint(int grey, int alpha) { tint(rgba(grey, alpha)); }
    void tint(int red, int green, int blue, int alpha) { tint(rgba(red, green, blue, alpha)); }
    void tint(color_t color) { getGraphicsComponent().tint(color); }
    void noTint() { getGraphicsComponent().noTint(); }

    void bezierDetail(uint32_t detail) { getGraphicsComponent().bezierDetail(detail); }
    void curveTightness(float tightness) { getGraphicsComponent().curveTightness(tightness); }
    void curveDetail(uint32_t detail) { getGraphicsComponent().curveDetail(detail); }

    void textFont(Font font) { getGraphicsComponent().textFont(font); }
    void noTextFont() { getGraphicsComponent().noTextFont(); }
    void textSize(float size) { getGraphicsComponent().textSize(size); }
    void textLetterSpacing(float spacing) { getGraphicsComponent().textLetterSpacing(spacing); }
    void textLineSpacing(float spacing) { getGraphicsComponent().textLineSpacing(spacing); }
    void textAlign(TextAlign textAlign) { getGraphicsComponent().textAlign(textAlign); }
    void textWrap(TextWrap textWrap) { getGraphicsComponent().textWrap(textWrap); }

    void shader(const Shader& shader) { getGraphicsComponent().shader(shader); }
    void noShader() { getGraphicsComponent().noShader(); }
    void blendMode(const BlendMode& blendMode) { getGraphicsComponent().blendMode(blendMode); }

    void setUniform(const std::string& name, const UniformVariable& variable) { getGraphicsComponent().setUniform(name, variable); }
    void setUniform(const Shader& shader, const std::string& name, const UniformVariable& variable) { getGraphicsComponent().setUniform(shader, name, variable); }

    void background(int grey, int alpha) { background(grey, grey, grey, alpha); }
    void background(int red, int green, int blue, int alpha) { background(rgba(red, green, blue, alpha)); }
    void background(color_t color) { getGraphicsComponent().background(color); }

    void beginShape() { getGraphicsComponent().beginShape(); }
    void endShape(ShapeType shapeType, bool close) { getGraphicsComponent().endShape(shapeType, close); }
    void vertex(float x, float y) { getGraphicsComponent().vertex(x, y, 0.0f, 0.0f); }
    void vertex(float x, float y, float u, float v) { getGraphicsComponent().vertex(x, y, u, v); }
    void curveVertex(float x, float y) { getGraphicsComponent().curveVertex(x, y); }

    void rect(float left, float top, float width, float height) { getGraphicsComponent().rect(left, top, width, height); }
    void rect(float left, float top, float width, float height, const BorderRadius& borderRadius) { getGraphicsComponent().rect(left, top, width, height, borderRadius); }
    void square(float left, float top, float size) { rect(left, top, size, size); }
    void ellipse(float centerX, float centerY, float radiusX, float radiusY) { getGraphicsComponent().ellipse(centerX, centerY, radiusX, radiusY); }
    void circle(float centerX, float centerY, float radius) { ellipse(centerX, centerY, radius, radius); }
    void point(float x, float y) { getGraphicsComponent().point(x, y); }
    void triangle(float x1, float y1, float x2, float y2, float x3, float y3) { getGraphicsComponent().triangle(x1, y1, x2, y2, x3, y3); }
    void line(float x1, float y1, float x2, float y2) { getGraphicsComponent().line(x1, y1, x2, y2); }
    void arc(float centerX, float centerY, float radiusX, float radiusY, float startAngle, float sweepAngle, ArcMode arcMode) { getGraphicsComponent().arc(centerX, centerY, radiusX, radiusY, startAngle, sweepAngle, arcMode); }
    void bezier(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4) { getGraphicsComponent().bezier(x1, y1, x2, y2, x3, y3, x4, y4); }
    void curve(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4) { getGraphicsComponent().curve(x1, y1, x2, y2, x3, y3, x4, y4); }
    void image(const Texture* texture, float left, float top, float width, float height) { getGraphicsComponent().image(*texture, left, top, width, height); }
    void text(std::string_view text, float x, float y, std::optional<float> maxWidth) { getGraphicsComponent().text(text, x, y, maxWidth); }
} // namespace p5cpp

int main()
{
    using namespace p5cpp;

    engine = Engine::create();
    engine->addModule(std::make_unique<FrameModule>());
    engine->addModule(std::make_unique<WindowModule>());
    engine->addModule(std::make_unique<InputModule>());
    engine->addModule(std::make_unique<GraphicsModule>());
    engine->addModule(std::make_unique<SketchModule>());

    engine->run();

    return 0;
}
