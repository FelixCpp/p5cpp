#include <p5cpp/p5cpp.hpp>

using namespace p5cpp;

inline static constexpr std::string_view shadowEffect = R"(
uniform vec4 u_ShadowColor;

vec4 effect(vec2 uv, vec2 texelSize, sampler2D tex)
{
    vec4 color = texture(tex, uv);
    float alpha = color.a;

    return vec4(u_ShadowColor.rgb, alpha * u_ShadowColor.a);
}
)";

struct ShadowRenderer
{
    Framebuffer layer;
    Framebuffer shadow;
    Framebuffer blurScratch;

    Shader shadowShader;
    Shader blurShader;

    int width;
    int height;

    void setup(int w, int h)
    {
        width = w;
        height = h;

        layer = createFramebuffer(width, height);
        shadow = createFramebuffer(width, height);
        blurScratch = createFramebuffer(width, height);

        shadowShader = loadEffectShader(shadowEffect);
        blurShader = loadBlurShader();
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
        // 2. Silhouette in Schattenfarbe einfärben
        //
        pushCanvas(shadow);
        {
            // BlendMode::none: this pass replaces shadow's content wholesale rather than
            // compositing over it - image() defaults to alpha blending, which would mix
            // the new content with whatever's already there instead of overwriting it.
            blendMode(BlendMode::none);
            shader(shadowShader);
            setUniform(shadowShader, "u_ShadowColor", uniform(0.0f, 0.0f, 0.0f, 1.0f));
            image(layer.colorTexture, 0, 0, width, height);
            noShader();
        }
        popCanvas();

        //
        // 3. Weichzeichnen abhängig von Elevation (separables Gauß-Blur, zwei Durchgänge)
        //
        const float texelX = 1.0f / static_cast<float>(width);
        const float texelY = 1.0f / static_cast<float>(height);
        setUniform(blurShader, "u_TexelSize", uniform(texelX, texelY));
        setUniform(blurShader, "u_Radius", uniform(elevation * 2.0f));

        pushCanvas(blurScratch);
        {
            // Same reasoning as above: blurScratch is never background()-cleared, so
            // without BlendMode::none this pass would blend the new blur pass over
            // whatever was left in blurScratch from the previous frame.
            blendMode(BlendMode::none);
            shader(blurShader);
            setUniform(blurShader, "u_Direction", uniform(1.0f, 0.0f));
            image(shadow.colorTexture, 0, 0, width, height);
            noShader();
        }
        popCanvas();

        pushCanvas(shadow);
        {
            // shadow already holds this frame's (unblurred-in-Y) color pass output at
            // this point - without BlendMode::none, the vertical blur would composite
            // over that sharp silhouette instead of replacing it, propping up the alpha
            // near the edge and making the blur look harder/less soft than it should.
            blendMode(BlendMode::none);
            shader(blurShader);
            setUniform(blurShader, "u_Direction", uniform(0.0f, 1.0f));
            image(blurScratch.colorTexture, 0, 0, width, height);
            noShader();
        }
        popCanvas();

        //
        // 4. Schatten zeichnen
        //
        image(shadow.colorTexture, 0, 0, width, height);

        //
        // 5. Original zeichnen
        //
        image(layer.colorTexture, 0, 0, width, height);
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

        shadows.drawShadow(
            [] {
                translate(getMouseX(), getMouseY());
                rotate(radians(45.0f));
                noFill();
                stroke(255);
                strokeWeight(4.0f);
                rect(-100.0f, -100.0f, 200.0f, 200.0f, BorderRadius::circular(12.0f));
                // ellipse(getMouseX(), getMouseY(), 100.0f, 100.0f);
                // rect(getMouseX(), getMouseY(), 120.0f, 80.0f);
                // fill(255, 0, 0);
                // textSize(64.0f);
                // text("Hello, World!", 300.0f, 150.0f);
            },
            1.0f
        );
    }
};

std::unique_ptr<Sketch> p5cpp::createSketch()
{
    return std::make_unique<ShadowRendering>();
}
