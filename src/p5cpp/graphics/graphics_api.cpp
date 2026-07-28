#include <p5cpp/application/engine.hpp>
#include <p5cpp/application/app_context.hpp>
#include <p5cpp/graphics/graphics_component.hpp>

namespace p5cpp
{
    extern std::unique_ptr<Engine> engine;
}

namespace p5cpp
{
    GraphicsComponent& getGraphicsComponent()
    {
        static GraphicsComponent* s_graphicsComponent = nullptr;
        static Engine* s_engine = nullptr;
        if (s_engine != engine.get()) {
            s_engine = engine.get();
            s_graphicsComponent = &engine->getContext().require<GraphicsComponent>();
        }
        return *s_graphicsComponent;
    }
} // namespace p5cpp

namespace p5cpp
{
    void pushCanvas(const Framebuffer& framebuffer) { getGraphicsComponent().pushCanvas(framebuffer); }
    void popCanvas() { getGraphicsComponent().popCanvas(); }
    uint2 getCanvasSize() { return getGraphicsComponent().getCanvasSize(); }
    Pixels loadPixels() { return getGraphicsComponent().loadPixels(); }
    void updatePixels(const Pixels& pixels) { getGraphicsComponent().updatePixels(pixels); }

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

    void push() { getGraphicsComponent().push(); }
    void pop() { getGraphicsComponent().pop(); }

    void noFill() { getGraphicsComponent().noFill(); }
    void fill(color_t color) { getGraphicsComponent().fill(color); }
    void fill(int red, int green, int blue, int alpha) { fill(rgba(red, green, blue, alpha)); }
    void fill(int grey, int alpha) { fill(grey, grey, grey, alpha); }

    void noStroke() { getGraphicsComponent().noStroke(); }
    void stroke(color_t color) { getGraphicsComponent().stroke(color); }
    void stroke(int red, int green, int blue, int alpha) { stroke(rgba(red, green, blue, alpha)); }
    void stroke(int grey, int alpha) { stroke(grey, grey, grey, alpha); }

    void strokeWeight(float strokeWeight) { getGraphicsComponent().strokeWeight(strokeWeight); }
    void strokeCap(StrokeCap strokeCap) { getGraphicsComponent().strokeCap(strokeCap); }
    void strokeJoin(StrokeJoin strokeJoin) { getGraphicsComponent().strokeJoin(strokeJoin); }
    void miterLimit(float miterLimit) { getGraphicsComponent().miterLimit(miterLimit); }
    void roundJoinThreshold(float roundJoinThreshold) { getGraphicsComponent().roundJoinThreshold(roundJoinThreshold); }

    void noTint() { getGraphicsComponent().noTint(); }
    void tint(color_t color) { getGraphicsComponent().tint(color); }
    void tint(int red, int green, int blue, int alpha) { tint(rgba(red, green, blue, alpha)); }
    void tint(int grey, int alpha) { tint(rgba(grey, alpha)); }

    void textureMode(TextureMode textureMode) { getGraphicsComponent().textureMode(textureMode); }

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
    void textToPointsDetail(uint32_t detail) { getGraphicsComponent().textToPointsDetail(detail); }
    void textToPointsSpacing(float spacing) { getGraphicsComponent().textToPointsSpacing(spacing); }

    void shader(const Shader& shader) { getGraphicsComponent().shader(shader); }
    void noShader() { getGraphicsComponent().noShader(); }
    void blendMode(const BlendMode& blendMode) { getGraphicsComponent().blendMode(blendMode); }

    void smooth(uint32_t samples) { getGraphicsComponent().smooth(samples); }
    void noSmooth() { getGraphicsComponent().noSmooth(); }

    void setUniform(const std::string& name, const UniformVariable& variable) { getGraphicsComponent().setUniform(name, variable); }
    void setUniform(const Shader& shader, const std::string& name, const UniformVariable& variable) { getGraphicsComponent().setUniform(shader, name, variable); }

    void background(color_t color) { getGraphicsComponent().background(color); }
    void background(int red, int green, int blue, int alpha) { background(rgba(red, green, blue, alpha)); }
    void background(int grey, int alpha) { background(grey, grey, grey, alpha); }

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
    void image(const Texture& texture, float left, float top, float width, float height) { getGraphicsComponent().image(texture, left, top, width, height); }
    void image(const Texture& texture, float left, float top, float width, float height, float sx, float sy, float sWidth, float sHeight) { getGraphicsComponent().image(texture, left, top, width, height, sx, sy, sWidth, sHeight); }
    void text(std::string_view text, float x, float y) { getGraphicsComponent().text(text, x, y); }
    void text(std::string_view text, float x, float y, float maxWidth) { getGraphicsComponent().text(text, x, y, maxWidth); }
    TextLayout textLayout(std::string_view text, float x, float y) { return getGraphicsComponent().layoutText(text, x, y); }
    TextLayout textLayout(std::string_view text, float x, float y, float maxWidth) { return getGraphicsComponent().layoutText(text, x, y, maxWidth); }

    std::vector<TextContour> textToPoints(std::string_view text, float x, float y) { return getGraphicsComponent().textToPoints(text, x, y); }
    std::vector<TextContour> textToPoints(std::string_view text, float x, float y, float maxWidth) { return getGraphicsComponent().textToPoints(text, x, y, maxWidth); }

    RenderGroup buildRenderGroup(const std::function<void()>& buildFn) { return getGraphicsComponent().buildRenderGroup(buildFn); }
    void drawRenderGroup(const RenderGroup& group) { getGraphicsComponent().drawRenderGroup(group); }
    void drawRenderGroup(const RenderGroup& group, float x, float y)
    {
        pushMatrix();
        translate(x, y);
        drawRenderGroup(group);
        popMatrix();
    }
} // namespace p5cpp
