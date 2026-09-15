#include <p5cpp/p5cpp.hpp>

using namespace p5;

inline static constexpr std::string_view WGSL_GRAYSCALE_SHADER_SOURCE = R"(
fn effect(
    color: vec4f,
    image: texture_2d<f32>,
    imageSampler: sampler,
    texCoord: vec2f,
    screenCoord: vec2f
) -> vec4f {
    let sampledColor: vec4f = textureSample(image, imageSampler, texCoord);
    let luminance: f32 = dot(sampledColor.rgb, vec3f(0.299, 0.587, 0.114));
    return vec4f(luminance, luminance, luminance, sampledColor.a);
}
)";

inline static constexpr std::string_view WGSL_SOBEL_EDGE_DETECTION_SHADER_SOURCE = R"(
struct Uniforms {
    u_Width: f32,
    u_Height: f32,
};

@group(2) @binding(0) var<uniform> u_Uniforms: Uniforms;

fn make_kernel(n: ptr<function, array<vec4f, 9>>, image: texture_2d<f32>, imageSampler: sampler, coord: vec2f, width: f32, height: f32) {
    let w: f32 = 1.0 / width;
    let h: f32 = 1.0 / height;

    (*n)[0] = textureSample(image, imageSampler, coord + vec2( -w, -h));
    (*n)[1] = textureSample(image, imageSampler, coord + vec2(0.0, -h));
    (*n)[2] = textureSample(image, imageSampler, coord + vec2(  w, -h));
    (*n)[3] = textureSample(image, imageSampler, coord + vec2( -w, 0.0));
    (*n)[4] = textureSample(image, imageSampler, coord);
    (*n)[5] = textureSample(image, imageSampler, coord + vec2(  w, 0.0));
    (*n)[6] = textureSample(image, imageSampler, coord + vec2( -w, h));
    (*n)[7] = textureSample(image, imageSampler, coord + vec2(0.0, h));
    (*n)[8] = textureSample(image, imageSampler, coord + vec2(  w, h));
}

fn effect(color: vec4f, image: texture_2d<f32>, imageSampler: sampler, texCoord: vec2f, screenCoord: vec2f) -> vec4f {
    var n: array<vec4f, 9>;
    make_kernel(&n, image, imageSampler, texCoord, u_Uniforms.u_Width, u_Uniforms.u_Height);

    let sobel_edge_h: vec4f = n[2] + (2.0 * n[5]) + n[8] - (n[0] + (2.0 * n[3]) + n[6]);
    let sobel_edge_v: vec4f = n[0] + (2.0 * n[1]) + n[2] - (n[6] + (2.0 * n[7]) + n[8]);
    let sobel: vec4f = sqrt((sobel_edge_h * sobel_edge_h) + (sobel_edge_v * sobel_edge_v));

    return vec4f(1.0 - sobel.rgb, 1.0);
}
)";

struct WorldMap : Sketch
{
    Texture worldMapTexture = loadTexture("assets/simple_map.jpg").value();
    Shader grayscaleShader = loadShaderFromMemory(WGSL_GRAYSCALE_SHADER_SOURCE).value();
    Shader sobelEdgeDetectionShader = loadShaderFromMemory(WGSL_SOBEL_EDGE_DETECTION_SHADER_SOURCE).value();
    Texture grayscaledWorldMapTexture;
    Texture edgeDetectedWorldMapTexture;

    void setup() override
    {
        const uint32_t mapWidth = worldMapTexture.size.x;
        const uint32_t mapHeight = worldMapTexture.size.y;
        const float aspectRatio = static_cast<float>(mapWidth) / static_cast<float>(mapHeight);
        const int windowWidth = std::min(1280u, mapWidth);
        const int windowHeight = static_cast<int>(windowWidth / aspectRatio);

        setWindowSize(windowWidth, windowHeight);
        setWindowResizable(false);

        {
            Graphics graphics = createGraphics(mapWidth, mapHeight).value();

            pushGraphics(graphics);
            shader(grayscaleShader);
            image(worldMapTexture, 0, 0, getWidth(), getHeight());
            noShader();
            popGraphics();
            grayscaledWorldMapTexture = graphics.colorTexture;
        }

        {
            Graphics graphics = createGraphics(mapWidth, mapHeight).value();
            pushGraphics(graphics);
            shader(sobelEdgeDetectionShader);
            setUniform("u_Width", static_cast<float>(mapWidth));
            setUniform("u_Height", static_cast<float>(mapHeight));
            image(grayscaledWorldMapTexture, 0, 0, getWidth(), getHeight());
            noShader();
            popGraphics();

            edgeDetectedWorldMapTexture = graphics.colorTexture;
        }
    }

    void draw() override
    {
        background(rgba(31, 31, 51));
        const float width = static_cast<float>(getWidth());
        const float height = static_cast<float>(getHeight());

        texture(edgeDetectedWorldMapTexture);
        beginShape(ShapeMode::quads);
        vertex(0.0f, 0.0f, 0.0f, 0.0f);
        vertex(width, 0.0f, 1.0f, 0.0f);
        vertex(width, height, 1.0f, 1.0f);
        vertex(0.0f, height, 0.0f, 1.0f);
        endShape();
        noTexture();
    }
};

SketchSpec p5::createSpec()
{
    return SketchSpec {
        .sketch = [] {
            return std::make_unique<WorldMap>();
        }
    };
}
