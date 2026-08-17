#include <p5cpp/p5cpp.hpp>

using namespace p5;

// Demonstrates smooth()/noSmooth() (MSAA antialiasing for the default canvas). Also press Space
// at any time to toggle it interactively and compare the circle's edge visually.
struct SmoothDemoSketch : public Sketch
{
    void setup() override
    {
        setWindowSize(300, 300);
    }

    void draw() override
    {
        const bool spaceDown = isKeyDown(Key::Space);
        if (spaceDown and not m_spaceWasDown) {
            m_smoothEnabled = not m_smoothEnabled;
            if (m_smoothEnabled) {
                smooth(8);
            } else {
                noSmooth();
            }
            info("smooth: {}", m_smoothEnabled ? "on (8x MSAA)" : "off");
        }
        m_spaceWasDown = spaceDown;

        background(rgba(25));
        noStroke();
        fill(rgba(0, 120, 255));
        circle(150, 150, 120);

        // Self-check: scan across the circle's steepest edge (its horizontal diameter, x=30) a
        // few frames after each state settles, and verify noSmooth() produces a hard binary
        // transition while smooth() produces at least one blended intermediate pixel there.
        ++m_frameCount;
        if (m_frameCount == 5) {
            verifyEdge(/*expectBlend*/ false);
            noSmooth(); // in case Space was pressed already
            m_smoothEnabled = false;
        } else if (m_frameCount == 10) {
            smooth(8);
            m_smoothEnabled = true;
        } else if (m_frameCount == 15) {
            verifyEdge(/*expectBlend*/ true);
        }
    }

private:
    void verifyEdge(bool expectBlend)
    {
        const Pixels pixels = loadPixels();
        const color_t background = getPixel(pixels, 20, 150);
        const color_t fill = getPixel(pixels, 39, 150);
        const color_t boundary = getPixel(pixels, 30, 150);
        const bool isBlended = boundary != background and boundary != fill;
        const bool ok = isBlended == expectBlend;
        info("smooth={}: edge is {} — {}", expectBlend, isBlended ? "blended" : "hard", ok ? "OK" : "FAILED");
    }

    bool m_smoothEnabled = false;
    bool m_spaceWasDown = false;
    int m_frameCount = 0;
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<SmoothDemoSketch>();
}
