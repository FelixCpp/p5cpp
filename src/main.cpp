#include "p5.hpp"

#include <memory>
#include <algorithm>

using namespace p5;

static constexpr const char* grayscaleFragment = R"(
    #version 410 core
    layout(location = 0) out vec4 o_Color;
    in vec2 v_TexCoord;
    uniform sampler2D u_Screen;
    void main() {
        o_Color = texture(u_Screen, v_TexCoord);
        o_Color = vec4(vec3(dot(o_Color.rgb, vec3(0.299, 0.587, 0.114))), o_Color.a);
    }
)";

static constexpr const char* blurFragment = R"(
    #version 410 core
    layout(location = 0) out vec4 o_Color;
    in vec2 v_TexCoord;
    uniform sampler2D u_Screen;
    uniform float u_KernelSize;
    void main() {
        vec2 texelSize = 1.0 / textureSize(u_Screen, 0);
        float kernelSize = u_KernelSize;
        vec4 color = vec4(0.0);
        for (float x = -kernelSize; x <= kernelSize; x++) {
            for (float y = -kernelSize; y <= kernelSize; y++) {
                color += texture(u_Screen, v_TexCoord + vec2(x, y) * texelSize);
            }
        }
        o_Color = color / pow((kernelSize * 2.0 + 1.0), 2.0);
    }
)";

static constexpr const char* invertFragment = R"(
    #version 410 core
    layout(location = 0) out vec4 o_Color;
    in vec2 v_TexCoord;
    uniform sampler2D u_Screen;
    void main() {
        o_Color = texture(u_Screen, v_TexCoord);
        o_Color = vec4(vec3(1.0) - o_Color.rgb, o_Color.a);
    }
)";

struct Starfield : p5::Sketch
{
public:
    std::shared_ptr<Font> font;
    std::shared_ptr<Canvas> blurCanvas = createCanvas(800, 600);
    std::shared_ptr<Shader> grayscaleShader = loadFilterShader(blurFragment);
    std::shared_ptr<Shader> invertShader = loadFilterShader(invertFragment);

    void setup() override
    {
        font = loadFont("Skia.ttf");
    }

    void draw() override
    {
        background(51);

        pushCanvas(blurCanvas);
        {
            background(0);
            setUniform(uniform("u_KernelSize", 3.0f));
            textSize(42.0f);
            textAlign(HorizontalTextAlign::center, VerticalTextAlign::center);
            text("Ich bin hier", 400.0f, 300.0f);
        }
        popCanvas();

        image(blurCanvas->getTextureId(), 0.0f, 0.0f, 400.0f, 600.0f);

        pushCanvas(blurCanvas);
        {
            background(0);
            setUniform(uniform("u_KernelSize", 15.0f));
            textSize(42.0f);
            textAlign(HorizontalTextAlign::center, VerticalTextAlign::center);
            text("Ich bin hier 123", 400.0f, 300.0f);
        }
        popCanvas();

        image(blurCanvas->getTextureId(), 400.0f, 0.0f, 400.0f, 600.0f);
    }

    void destroy() override
    {
    }

    void mousePressed(int x, int y) override
    {
    }

    color_t hsbToRgb(float h, float s, float b)
    {
        h = std::fmod(h, 360.0f) / 60.0f;
        s = std::clamp(s, 0.0f, 1.0f);
        b = std::clamp(b, 0.0f, 1.0f);

        const float c = b * s;
        const float x = c * (1.0f - std::fabs(std::fmod(h, 2.0f) - 1.0f));
        const float m = b - c;

        float r = 0.0f;
        float g = 0.0f;
        float bl = 0.0f;

        if (h < 1.0f) {
            r = c;
            g = x;
            bl = 0.0f;
        } else if (h < 2.0f) {
            r = x;
            g = c;
            bl = 0.0f;
        } else if (h < 3.0f) {
            r = 0.0f;
            g = c;
            bl = x;
        } else if (h < 4.0f) {
            r = 0.0f;
            g = x;
            bl = c;
        } else if (h < 5.0f) {
            r = x;
            g = 0.0f;
            bl = c;
        } else {
            r = c;
            g = 0.0f;
            bl = x;
        }

        return color(
            static_cast<int>((r + m) * 255),
            static_cast<int>((g + m) * 255),
            static_cast<int>((bl + m) * 255)
        );
    }
};

namespace p5
{
    std::unique_ptr<p5::Sketch> createSketch()
    {
        return std::make_unique<Starfield>();
    }
} // namespace p5
