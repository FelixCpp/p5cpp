#include <cfloat>
#include <p5.hpp>

using namespace p5;

inline static constexpr std::string_view dummyText = R"(
Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.
)";

struct PixelTestSketch : Sketch
{

    void setup() override
    {
        setWindowSize(1024, 1024);
    }

    void event(const WindowEvent& e) override
    {
    }

    void draw() override
    {
        background(21);
        textSize(42.0f);
        textWrap(TextWrap::word);
        text(dummyText, 0.0f, 0.0f, static_cast<float>(getMouseX()));

        stroke(255, 0, 0);
        strokeWeight(15.0f);
        line(getMouseX(), 0.0f, getMouseX(), static_cast<float>(getHeight()));
    }
};

namespace p5
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<PixelTestSketch>();
    }
} // namespace p5
