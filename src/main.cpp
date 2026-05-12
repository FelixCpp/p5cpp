#include "p5.hpp"

#include <memory>

using namespace p5;

inline static constexpr const char* lightShaderVertexSource = R"(
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

inline static constexpr const char* lightShaderFragmentSource = R"(
    #version 410 core

    layout (location = 0) out vec4 o_Color;

    in vec2 v_TexCoord;
    in vec4 v_Color;
    in float v_TexIndex;

    uniform vec2 u_LightPos;
    uniform float u_LightRadius;

    uniform sampler2D u_Textures[8];

    void main() {
        vec4 texColor = vec4(1.0);
        switch(int(v_TexIndex)) {
            case 0: texColor = texture(u_Textures[0], v_TexCoord); break;
            case 1: texColor = texture(u_Textures[1], v_TexCoord); break;
            case 2: texColor = texture(u_Textures[2], v_TexCoord); break;
            case 3: texColor = texture(u_Textures[3], v_TexCoord); break;
            case 4: texColor = texture(u_Textures[4], v_TexCoord); break;
            case 5: texColor = texture(u_Textures[5], v_TexCoord); break;
            case 6: texColor = texture(u_Textures[6], v_TexCoord); break;
            case 7: texColor = texture(u_Textures[7], v_TexCoord); break;
            default: break;
        }

        vec2 fragPos = gl_FragCoord.xy;
        vec2 toLight = u_LightPos - fragPos;
        float distance = length(toLight);
        float attenuation = clamp(1.0 - distance / u_LightRadius, 0.0, 1.0);
        vec4 lightColor = vec4(attenuation, attenuation, attenuation, 1.0);
        o_Color = v_Color * texColor * lightColor;
    }
)";

struct Starfield : p5::Sketch
{
public:
    std::shared_ptr<Font> font;
    std::shared_ptr<Canvas> scene;
    std::shared_ptr<Shader> lightShader;

    void setup() override
    {
        font = loadFont("Skia.ttf");

        lightShader = loadShader(lightShaderVertexSource, lightShaderFragmentSource);

        scene = createCanvas(1600, 1200);
        pushCanvas(scene);
        textFont(font);
        background(21);
        textSize(64.0f);
        textAlign(HorizontalTextAlign::center, VerticalTextAlign::center);
        fill(0, 150, 190);
        text("Move the mouse to control the light source", 800.0f, 300.0f);
        text("Hello, p5!", 800.0f, 600.0f);
        popCanvas();
    }

    float time = 0.0f;

    void draw() override
    {
        time += 0.05f;
        float x = noise(time * 0.1f) * 800.0f + 800.0f;
        float y = noise((time + 1000.0f) * 0.1f) * 600.0f + 600.0f;

        background(0);
        shader(lightShader);
        setUniform("u_LightPos", x, y);
        setUniform("u_LightRadius", 600.0f);
        image(scene->getTextureId(), 0.0f, 0.0f, 1600.0f, 1200.0f);
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
