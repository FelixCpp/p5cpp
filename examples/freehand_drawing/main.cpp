#include <p5cpp/p5cpp.hpp>

namespace
{
    using namespace p5cpp;

    inline constexpr int W = 900;
    inline constexpr int H = 600;

    struct FreehandDrawingSketch : Sketch
    {
        Font font;
        Framebuffer canvas;

        void setup() override
        {
            setWindowSize(W, H);
            setWindowTitle("Freehand Drawing — mouse drag demo");
            font = loadFont("example_assets/MapleMono-NF-Regular.ttf");
            textFont(font);

            canvas = createFramebuffer(W, H);
            clearCanvas();
        }

        void clearCanvas()
        {
            pushCanvas(canvas);
                background(250);
            popCanvas();
        }

        void event(const WindowEvent& windowEvent) override
        {
            if (windowEvent.type == EventType::keyPress && windowEvent.keyEvent.key == Key::c) {
                clearCanvas();
            }
        }

        void draw() override
        {
            // isMouseDragging(button) is only true while that button is held down
            // *and* the mouse actually moved — exactly what Processing's
            // mouseDragged() callback fires for. Strokes are drawn straight into
            // the persistent offscreen canvas, one segment per frame, instead of
            // clearing/redrawing it every frame like background() would.
            if (isMouseDragging(MouseButton::left)) {
                pushCanvas(canvas);
                    stroke(30, 30, 30);
                    strokeWeight(4.0f);
                    line(static_cast<float>(getPMouseX()), static_cast<float>(getPMouseY()), static_cast<float>(getMouseX()), static_cast<float>(getMouseY()));
                popCanvas();
            }

            if (isMouseDragging(MouseButton::right)) {
                pushCanvas(canvas);
                    stroke(250);
                    strokeWeight(24.0f); // eraser — thicker, background-coloured
                    line(static_cast<float>(getPMouseX()), static_cast<float>(getPMouseY()), static_cast<float>(getMouseX()), static_cast<float>(getMouseY()));
                popCanvas();
            }

            background(20);
            image(canvas.colorTexture, 0, 0, static_cast<float>(W), static_cast<float>(H));

            fill(0);
            textSize(16.0f);
            textAlign(TextAlign::topLeft);
            text("Left-drag to draw, right-drag to erase, C to clear", 20, 20);
        }
    };
} // namespace

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<FreehandDrawingSketch>();
    }
} // namespace p5cpp
