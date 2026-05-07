#include "p5.hpp"

#include <memory>
#include <vector>

using namespace p5;

struct Starfield : p5::Sketch
{
public:
    std::shared_ptr<Font> font = loadFont("Skia.ttf");

    void setup() override
    {
    }

    void draw() override
    {
        background(51);

        // color_t lightBlue = color(40, 120, 200);
        // color_t lighterBlueBg = color(40, 160, 220);
        //
        // fill(lighterBlueBg);
        // stroke(lightBlue);
        // strokeWeight(15.0f);
        // fill(255, 0, 0);
        // rect(mouseX - 150.0f, mouseY - 50.0f, 300.0f, 100.0f);
        //
        textAlign(HorizontalTextAlign::center, VerticalTextAlign::center);
        fill(255);
        text("Hi, Welt!", mouseX, mouseY);

        textFont(font);
        // noTextFont();
        textSize(48.0f);
        fill(255);
        textAlign(HorizontalTextAlign::center, VerticalTextAlign::center);
        text(std::string("Click to add points!"), 400.0f, 300.0f);
        textAlign(HorizontalTextAlign::left, VerticalTextAlign::baseline);
        textSize(16.0f);
        text("Points: " + std::to_string(positions.size()), 10.0f, 20.0f);

        // const color_t orange = color(255, 120, 0);
        //
        // noFill();
        // stroke(orange);
        // strokeWeight(4.0f);
        // if (not positions.empty()) {
        //     beginShape();
        //
        //     for (size_t i = 3; i < positions.size(); i += 3) {
        //         bezier(positions[i - 3].x, positions[i - 3].y, positions[i - 2].x, positions[i - 2].y, positions[i - 1].x, positions[i - 1].y, positions[i].x, positions[i].y);
        //     }
        //
        //     endShape(false);
        // }
    }

    void destroy() override
    {
    }

    void mousePressed(int x, int y) override
    {
        positions.push_back(float2 {static_cast<float>(x), static_cast<float>(y)});
    }

    std::vector<float2> positions;
};

namespace p5
{
    std::unique_ptr<p5::Sketch> createSketch()
    {
        return std::make_unique<Starfield>();
    }
} // namespace p5
