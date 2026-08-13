#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/graphics.hpp>
#include <p5cpp/application/kernel.hpp>

namespace p5
{
    namespace
    {
        Graphics& graphics()
        {
            return getKernel().getContext().require<Graphics>();
        }
    } // namespace

    void pushFramebuffer(std::shared_ptr<Framebuffer> framebuffer) { graphics().pushFramebuffer(std::move(framebuffer)); }
    void popFramebuffer() { graphics().popFramebuffer(); }
    const uint2& getFramebufferSize() { return graphics().getFramebufferSize(); }
    float getWidth() { return static_cast<float>(graphics().getFramebufferSize().x); }
    float getHeight() { return static_cast<float>(graphics().getFramebufferSize().y); }
    void push() { graphics().push(); }
    void pop() { graphics().pop(); }
    void pushState() { graphics().pushState(); }
    void popState() { graphics().popState(); }
    void pushMatrix() { graphics().pushMatrix(); }
    void popMatrix() { graphics().popMatrix(); }
    void applyMatrix(const matrix4x4& matrix) { graphics().applyMatrix(matrix); }
    void setMatrix(const matrix4x4& matrix) { graphics().setMatrix(matrix); }
    void translate(float x, float y) { graphics().translate(x, y); }
    void scale(float x, float y) { graphics().scale(x, y); }
    void rotate(float radians) { graphics().rotate(radians); }
    void fill(color_t color) { graphics().fill(color); }
    void noFill() { graphics().noFill(); }
    void stroke(color_t color) { graphics().stroke(color); }
    void noStroke() { graphics().noStroke(); }
    void strokeWeight(float weight) { graphics().strokeWeight(weight); }
    void strokeCap(StrokeCap cap) { graphics().strokeCap(cap); }
    void strokeJoin(StrokeJoin join) { graphics().strokeJoin(join); }
    void strokeMiterLimit(float limit) { graphics().strokeMiterLimit(limit); }
    void strokeRoundJoinThreshold(float threshold) { graphics().strokeRoundJoinThreshold(threshold); }
    void curveTightness(float tightness) { graphics().curveTightness(tightness); }
    void blendMode(const BlendMode& blendMode) { graphics().blendMode(blendMode); }
    void clip(float x, float y, float width, float height) { graphics().clip(x, y, width, height); }
    void noClip() { graphics().noClip(); }
    void shader(std::shared_ptr<Shader> shader) { graphics().shader(shader); }
    void noShader() { graphics().noShader(); }
    void background(color_t color) { graphics().background(color); }
    void rect(float left, float top, float width, float height) { graphics().rect(left, top, width, height); }
    void rect(float left, float top, float width, float height, const BorderRadius& borderRadius) { graphics().rect(left, top, width, height, borderRadius); }
    void square(float left, float top, float size) { graphics().square(left, top, size); }
    void ellipse(float centerX, float centerY, float radiusX, float radiusY) { graphics().ellipse(centerX, centerY, radiusX, radiusY); }
    void circle(float centerX, float centerY, float radius) { graphics().circle(centerX, centerY, radius); }
    void line(float x1, float y1, float x2, float y2) { graphics().line(x1, y1, x2, y2); }
    void triangle(float x1, float y1, float x2, float y2, float x3, float y3) { graphics().triangle(x1, y1, x2, y2, x3, y3); }
    void point(float x, float y) { graphics().point(x, y); }
    void beginShape(ShapeMode mode) { graphics().beginShape(mode); }
    void vertex(float x, float y) { graphics().vertex(x, y); }
    void vertex(float x, float y, float u, float v) { graphics().vertex(x, y, u, v); }
    void bezierVertex(float controlX1, float controlY1, float controlX2, float controlY2, float x, float y) { graphics().bezierVertex(controlX1, controlY1, controlX2, controlY2, x, y); }
    void quadraticVertex(float controlX, float controlY, float x, float y) { graphics().quadraticVertex(controlX, controlY, x, y); }
    void curveVertex(float x, float y) { graphics().curveVertex(x, y); }
    void endShape(bool close) { graphics().endShape(close); }
    void bezier(float x1, float y1, float controlX1, float controlY1, float controlX2, float controlY2, float x2, float y2) { graphics().bezier(x1, y1, controlX1, controlY1, controlX2, controlY2, x2, y2); }
    void curve(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4) { graphics().curve(x1, y1, x2, y2, x3, y3, x4, y4); }
    void imageUVMode(TextureUVMode mode) { graphics().imageUVMode(mode); }
    void textureFilter(TextureFilter filter) { graphics().textureFilter(filter); }
    void textureWrap(TextureWrap wrap) { graphics().textureWrap(wrap); }
    void image(std::shared_ptr<Texture> texture, float left, float top, float width, float height) { graphics().image(std::move(texture), left, top, width, height); }
    void image(std::shared_ptr<Texture> texture, float left, float top, float width, float height, float u1, float v1, float u2, float v2) { graphics().image(std::move(texture), left, top, width, height, u1, v1, u2, v2); }
} // namespace p5
