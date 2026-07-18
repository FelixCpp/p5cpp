#include <p5cpp/p5cpp.hpp>

using namespace p5cpp;

// Demonstrates Font::textToPoints(), the p5cpp equivalent of p5.js's font.textToPoints():
// it returns the outline of a piece of text as a list of closed contours (one per outer
// boundary / inner hole) instead of drawing it. Here each contour is drawn both as its
// outline and as a field of little circles sitting on every sampled point.
struct TextToPointsSketch : p5cpp::Sketch
{
    Font font;
    std::vector<TextContour> contours;

    void setup() override
    {
        setWindowSize(800, 500);
        font = loadFont("example_assets/MapleMono-NF-Regular.ttf");
        contours = font.textToPoints("p5cpp", 60.0f, 260.0f, 180, 50.0f);
    }

    void draw() override
    {
        background(20);

        // noFill();
        // stroke(80, 160, 255);
        // strokeWeight(1.5f);
        // for (const TextContour& contour : contours) {
        //     beginShape();
        //     for (const float2& p : contour) {
        //         vertex(p.x, p.y);
        //     }
        //     endShape(ShapeType::lineLoop, true);
        // }

        noStroke();
        fill(255, 200, 0);
        for (const TextContour& contour : contours) {
            for (const float2& p : contour) {
                circle(p.x, p.y, 2.5f);
            }
        }
    }
};

namespace p5cpp
{
    std::unique_ptr<p5cpp::Sketch> createSketch()
    {
        return std::make_unique<TextToPointsSketch>();
    }
} // namespace p5cpp
