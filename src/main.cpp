#include "p5.hpp"

#include <memory>

using namespace p5;

inline static constexpr const char* blurVertexSource = R"(
#version 410 core

layout (location = 0) in vec2 a_Position;
layout (location = 1) in vec2 a_TexCoord;
layout (location = 2) in vec4 a_Color;
layout (location = 3) in float a_TexIndex;

out vec2 v_TexCoord;
out vec4 v_Color;
out float v_TexIndex;

uniform mat4 u_ProjectionMatrix;

void main() {
    gl_Position = u_ProjectionMatrix * vec4(a_Position, 0.0, 1.0);

    v_TexCoord = a_TexCoord;
    v_Color = a_Color;
    v_TexIndex = a_TexIndex;
}
)";

inline static constexpr const char* blurFragmentSource = R"(
#version 410 core
layout (location = 0) out vec4 o_Color;
in vec2 v_TexCoord;
in vec4 v_Color;
in float v_TexIndex;
uniform sampler2D u_Textures[8];
uniform vec2 u_TexelSize;   // 1.0 / textureSize, per CPU setzen
uniform float u_Radius;       // Blur-Radius in Pixeln (z.B. 5)

vec4 sampleTexture(int index, vec2 uv) {
    switch(index) {
        case 0: return texture(u_Textures[0], uv);
        case 1: return texture(u_Textures[1], uv);
        case 2: return texture(u_Textures[2], uv);
        case 3: return texture(u_Textures[3], uv);
        case 4: return texture(u_Textures[4], uv);
        case 5: return texture(u_Textures[5], uv);
        case 6: return texture(u_Textures[6], uv);
        case 7: return texture(u_Textures[7], uv);
    }
    return vec4(0.0);
}

void main() {

    int texIndex = int(v_TexIndex);

    vec4 result = vec4(0.0);
    float weightSum = 0.0;

    for (int x = -u_Radius; x <= u_Radius; x++) {
        for (int y = -u_Radius; y <= u_Radius; y++) {
            vec2 offset = vec2(float(x), float(y)) * u_TexelSize;

            // Gaussian weight: e^(-(x²+y²) / (2σ²)), σ = Radius/2
            float sigma = float(u_Radius) / 2.0;
            float weight = exp(-float(x*x + y*y) / (2.0 * sigma * sigma));

            result += sampleTexture(texIndex, v_TexCoord + offset) * weight;
            weightSum += weight;
        }
    }

    result /= weightSum;
    o_Color = result * v_Color;
}
)";

#include <vector>

struct Starfield : p5::Sketch
{
public:
    std::shared_ptr<Font> font;
    std::shared_ptr<Shader> blurShader = loadShader(blurVertexSource, blurFragmentSource);

    std::shared_ptr<Texture> texture;

    void setup() override
    {
        font = loadFont("Skia.ttf");

        const int w = 400;
        const int h = 600;
        std::vector<uint8_t> data(w * h * 4);
        for (size_t i = 0; i < w * h; ++i) {
            data[i * 4 + 0] = static_cast<uint8_t>(rand() % 256);
            data[i * 4 + 1] = static_cast<uint8_t>(rand() % 256);
            data[i * 4 + 2] = static_cast<uint8_t>(rand() % 256);
            data[i * 4 + 3] = 255;
        }

        texture = loadTexture(w, h, data);
    }

    void draw() override
    {
        background(51);

        shader(blurShader);
        setUniform("u_TexelSize", uniform(1.0f / 400.0f, 1.0f / 600.0f));
        setUniform("u_Radius", uniform(5.0f));
        image(texture->getRendererId(), 0, 0, 400, 600);
        noShader();

        strokeWeight(10.0f);
        stroke(255);
        line(400, 0, 400, 600);

        shader(blurShader);
        setUniform("u_TexelSize", uniform(1.0f / 400.0f, 1.0f / 600.0f));
        setUniform("u_Radius", uniform(15.0f));
        image(texture->getRendererId(), 400, 0, 400, 600);
        noShader();
    }

    void destroy() override
    {
    }

    void mousePressed(int x, int y) override
    {
    }
};

namespace p5
{
    std::unique_ptr<p5::Sketch> createSketch()
    {
        return std::make_unique<Starfield>();
    }
} // namespace p5
