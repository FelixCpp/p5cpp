#include <p5cpp/p5cpp.hpp>

using namespace p5cpp;

inline static constexpr std::string_view shadowEffect = R"(
uniform vec4 u_ShadowColor;

vec4 effect(vec2 uv, vec2 texelSize, sampler2D tex)
{
    vec4 color = texture(tex, uv);

    // Nur Alpha des Objekts verwenden
    float alpha = color.a;

    return vec4(
        u_ShadowColor.rgb,
        alpha * u_ShadowColor.a
    );
}
)";

struct ShadowRenderer
{
    Framebuffer layer;
    Framebuffer shadow;

    Shader shadowShader;

    int width;
    int height;

    void setup(int w, int h)
    {
        width = w;
        height = h;

        layer = createFramebuffer(width, height);
        shadow = createFramebuffer(width, height);

        shadowShader = loadEffectShader(shadowEffect);
    }

    template <typename DrawFunction>
    void drawShadow(DrawFunction draw, float elevation)
    {
        //
        // 1. Objekt rendern
        //
        pushCanvas(layer);
        {
            background(0, 0);

            draw();
        }
        popCanvas();

        //
        // 2. Shadow Texture erzeugen
        //
        pushCanvas(shadow);
        {
            background(0, 0);
            setUniform(shadowShader, "u_ShadowColor", uniform(0.0f, 0.0f, 0.0f, 0.5f));
            image(*layer.getColorTexture(), 0, 0, width, height);

            effect(shadowShader);

            //
            // Blur abhängig von Elevation
            //
            filter(FilterType::blur, elevation * 2.0f);
        }
        popCanvas();

        //
        // 3. Schatten zeichnen
        //
        image(*shadow.getColorTexture(), elevation * 0.5f, elevation, width, height);

        //
        // 4. Original zeichnen
        //
        image(*layer.getColorTexture(), 0, 0, width, height);
    }
};

struct ShadowRendering : Sketch
{
    ShadowRenderer shadows;

    void setup() override
    {
        setWindowSize(800, 800);
        smooth(4);

        shadows.setup(800, 800);
    }

    void draw() override
    {
        background(255);

        // shadows.drawShadow(
        //     [] {
        noStroke();
        fill(0, 255, 0);
        rect(getMouseX(), getMouseY(), 120.0f, 80.0f);
        fill(255, 0, 0);
        textSize(64.0f);
        text("Hello, World!", 300.0f, 150.0f);
        // },
        // 0.5f
        // );
    }
};

std::unique_ptr<Sketch> p5cpp::createSketch()
{
    return std::make_unique<ShadowRendering>();
}
