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

    void pushFramebuffer(std::shared_ptr<Framebuffer> framebuffer)
    {
        graphics().pushFramebuffer(std::move(framebuffer));
    }

    void popFramebuffer()
    {
        graphics().popFramebuffer();
    }

    const uint2& getFramebufferSize()
    {
        return graphics().getFramebufferSize();
    }

    void push()
    {
        graphics().push();
    }

    void pop()
    {
        graphics().pop();
    }

    void pushState()
    {
        graphics().pushState();
    }

    void popState()
    {
        graphics().popState();
    }

    void pushMatrix()
    {
        graphics().pushMatrix();
    }

    void popMatrix()
    {
        graphics().popMatrix();
    }

    void applyMatrix(const matrix4x4& matrix)
    {
        graphics().applyMatrix(matrix);
    }

    void setMatrix(const matrix4x4& matrix)
    {
        graphics().setMatrix(matrix);
    }

    void translate(float x, float y)
    {
        graphics().translate(x, y);
    }

    void scale(float x, float y)
    {
        graphics().scale(x, y);
    }

    void rotate(float radians)
    {
        graphics().rotate(radians);
    }

    void fill(color_t color)
    {
        graphics().fill(color);
    }

    void noFill()
    {
        graphics().noFill();
    }

    void stroke(color_t color)
    {
        graphics().stroke(color);
    }

    void noStroke()
    {
        graphics().noStroke();
    }

    void strokeWeight(float weight)
    {
        graphics().strokeWeight(weight);
    }

    void strokeCap(StrokeCap cap)
    {
        graphics().strokeCap(cap);
    }

    void strokeJoin(StrokeJoin join)
    {
        graphics().strokeJoin(join);
    }

    void strokeMiterLimit(float limit)
    {
        graphics().strokeMiterLimit(limit);
    }

    void strokeRoundJoinThreshold(float threshold)
    {
        graphics().strokeRoundJoinThreshold(threshold);
    }

    void blendMode(const BlendMode& blendMode)
    {
        graphics().blendMode(blendMode);
    }

    void shader(std::shared_ptr<Shader> shader)
    {
        graphics().shader(shader);
    }

    void noShader()
    {
        graphics().noShader();
    }

    void setUniform(std::string_view name, float value) { graphics().setUniform(name, value); }
    void setUniform(std::string_view name, const float2& value) { graphics().setUniform(name, value); }
    void setUniform(std::string_view name, const float3& value) { graphics().setUniform(name, value); }
    void setUniform(std::string_view name, const float4& value) { graphics().setUniform(name, value); }
    void setUniform(std::string_view name, const matrix4x4& value) { graphics().setUniform(name, value); }
    void setUniform(std::string_view name, color_t value) { graphics().setUniform(name, value); }

    void background(color_t color)
    {
        graphics().background(color);
    }

    void rect(float left, float top, float width, float height)
    {
        graphics().rect(left, top, width, height);
    }

    void square(float left, float top, float size)
    {
        graphics().square(left, top, size);
    }

    void ellipse(float centerX, float centerY, float radiusX, float radiusY)
    {
        graphics().ellipse(centerX, centerY, radiusX, radiusY);
    }

    void circle(float centerX, float centerY, float radius)
    {
        graphics().circle(centerX, centerY, radius);
    }

    void line(float x1, float y1, float x2, float y2)
    {
        graphics().line(x1, y1, x2, y2);
    }

    void triangle(float x1, float y1, float x2, float y2, float x3, float y3)
    {
        graphics().triangle(x1, y1, x2, y2, x3, y3);
    }

    void point(float x, float y)
    {
        graphics().point(x, y);
    }
} // namespace p5
