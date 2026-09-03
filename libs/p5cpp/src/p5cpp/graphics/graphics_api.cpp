#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/canvas.hpp>
#include <p5cpp/graphics/graphics_plugin.hpp>
#include <p5cpp/application/kernel.hpp>

namespace p5
{
    namespace
    {
        Canvas& canvas()
        {
            return getKernel().getContext().require<Canvas>();
        }

        GraphicsPlugin& graphicsPlugin()
        {
            return getKernel().getContext().require<GraphicsPlugin>();
        }
    } // namespace

    void pushGraphics(Graphics graphics, bool extend) { canvas().pushGraphics(graphics, extend); }
    void popGraphics() { canvas().popGraphics(); }
    Graphics peekGraphics() { return canvas().peekGraphics(); }
    uint2 getGraphicsSize() { return canvas().getGraphicsSize(); }
    float getWidth() { return static_cast<float>(canvas().getGraphicsSize().x); }
    float getHeight() { return static_cast<float>(canvas().getGraphicsSize().y); }
    void smooth(uint32_t samples) { graphicsPlugin().smooth(samples); }
    void noSmooth() { graphicsPlugin().noSmooth(); }
    void flush() { canvas().flush(); }
    Pixels loadPixels() { return canvas().loadPixels(); }
    void updatePixels(const Pixels& pixels) { canvas().updatePixels(pixels); }
    void push(bool extend) { canvas().push(extend); }
    void pop() { canvas().pop(); }
    void pushState(bool extend) { canvas().pushState(extend); }
    void popState() { canvas().popState(); }
    void pushMatrix(bool extend) { canvas().pushMatrix(extend); }
    void popMatrix() { canvas().popMatrix(); }
    void applyMatrix(const matrix4x4& matrix) { canvas().applyMatrix(matrix); }
    void setMatrix(const matrix4x4& matrix) { canvas().setMatrix(matrix); }
    void translate(float x, float y) { canvas().translate(x, y); }
    void scale(float x, float y) { canvas().scale(x, y); }
    void rotate(float radians) { canvas().rotate(radians); }
    void fill(color_t color) { canvas().fill(color); }
    void noFill() { canvas().noFill(); }
    void stroke(color_t color) { canvas().stroke(color); }
    void noStroke() { canvas().noStroke(); }
    void strokeWeight(float weight) { canvas().strokeWeight(weight); }
    void strokeCap(StrokeCap cap) { canvas().strokeCap(cap); }
    void strokeJoin(StrokeJoin join) { canvas().strokeJoin(join); }
    void strokeMiterLimit(float limit) { canvas().strokeMiterLimit(limit); }
    void strokeRoundJoinThreshold(float threshold) { canvas().strokeRoundJoinThreshold(threshold); }
    void curveTightness(float tightness) { canvas().curveTightness(tightness); }
    void blendMode(const BlendMode& blendMode) { canvas().blendMode(blendMode); }
    void clip(float x, float y, float width, float height) { canvas().clip(x, y, width, height); }
    void noClip() { canvas().noClip(); }
    void shader(Shader shader) { canvas().shader(shader); }
    void noShader() { canvas().noShader(); }
    void background(color_t color) { canvas().background(color); }
    void rect(float left, float top, float width, float height) { canvas().rect(left, top, width, height); }
    void rect(float left, float top, float width, float height, const BorderRadius& borderRadius) { canvas().rect(left, top, width, height, borderRadius); }
    void square(float left, float top, float size) { canvas().square(left, top, size); }
    void ellipse(float centerX, float centerY, float radiusX, float radiusY) { canvas().ellipse(centerX, centerY, radiusX, radiusY); }
    void circle(float centerX, float centerY, float radius) { canvas().circle(centerX, centerY, radius); }
    void arc(float centerX, float centerY, float radiusX, float radiusY, float startAngle, float stopAngle, ArcMode mode) { canvas().arc(centerX, centerY, radiusX, radiusY, startAngle, stopAngle, mode); }
    void line(float x1, float y1, float x2, float y2) { canvas().line(x1, y1, x2, y2); }
    void triangle(float x1, float y1, float x2, float y2, float x3, float y3) { canvas().triangle(x1, y1, x2, y2, x3, y3); }
    void point(float x, float y) { canvas().point(x, y); }
    void beginShape(ShapeMode mode) { canvas().beginShape(mode); }
    void vertex(float x, float y) { canvas().vertex(x, y); }
    void vertex(float x, float y, float u, float v) { canvas().vertex(x, y, u, v); }
    void bezierVertex(float controlX1, float controlY1, float controlX2, float controlY2, float x, float y) { canvas().bezierVertex(controlX1, controlY1, controlX2, controlY2, x, y); }
    void quadraticVertex(float controlX, float controlY, float x, float y) { canvas().quadraticVertex(controlX, controlY, x, y); }
    void curveVertex(float x, float y) { canvas().curveVertex(x, y); }
    void endShape(bool close) { canvas().endShape(close); }
    void bezier(float x1, float y1, float controlX1, float controlY1, float controlX2, float controlY2, float x2, float y2) { canvas().bezier(x1, y1, controlX1, controlY1, controlX2, controlY2, x2, y2); }
    void curve(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4) { canvas().curve(x1, y1, x2, y2, x3, y3, x4, y4); }
    void textureUVMode(TextureUVMode mode) { canvas().textureUVMode(mode); }
    void textureFilter(TextureFilter filter) { canvas().textureFilter(filter); }
    void textureWrap(TextureWrap wrap) { canvas().textureWrap(wrap); }
    void image(Texture texture, float left, float top, float width, float height) { canvas().image(texture, left, top, width, height); }
    void image(Texture texture, float left, float top, float width, float height, float u1, float v1, float u2, float v2) { canvas().image(texture, left, top, width, height, u1, v1, u2, v2); }
    void image(const Graphics& graphics, float left, float top, float width, float height) { canvas().image(graphics.colorTexture, left, top, width, height); }
    void image(const Graphics& graphics, float left, float top, float width, float height, float u1, float v1, float u2, float v2) { canvas().image(graphics.colorTexture, left, top, width, height, u1, v1, u2, v2); }
    void textFont(Font font) { canvas().textFont(font); }
    void noTextFont() { canvas().noTextFont(); }
    void textSize(float pixels) { canvas().textSize(pixels); }
    void textAlign(TextAlignment alignment) { canvas().textAlign(alignment); }
    void textWrap(TextWrap wrap) { canvas().textWrap(wrap); }
    void textLeading(float pixels) { canvas().textLeading(pixels); }
    void noTextLeading() { canvas().noTextLeading(); }
    void textLetterSpacing(float pixels) { canvas().textLetterSpacing(pixels); }
    void text(std::string_view str, float x, float y, float maxWidth, float maxHeight) { canvas().text(str, x, y, maxWidth, maxHeight); }
    float textWidth(std::string_view str) { return canvas().textWidth(str); }
    rect2f textBounds(std::string_view str, float maxWidth) { return canvas().textBounds(str, maxWidth); }
    std::vector<TextPoint> textToPoints(std::string_view str, float x, float y, const TextToPointsOptions& options) { return canvas().textToPoints(str, x, y, options); }
} // namespace p5
