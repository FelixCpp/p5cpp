#include <p5cpp/p5cpp.hpp>
#include <p5cpp_camera/p5cpp_camera.hpp>

#include <format>

using namespace p5;
using namespace p5::camera;

struct Camera2DExample : Sketch
{
    static constexpr float gridSpacing = 100.0f;
    static constexpr int gridExtent = 15;
    static constexpr float2 pointOfInterest {500.0f, -300.0f};
    static constexpr float flyToZoom = 2.5f;

    bool interactionLocked = false;

    void setup() override
    {
        setWindowSize(900, 600);
        setCameraZoomRange(0.2f, 10.0f);
    }

    void draw() override
    {
        if (isKeyPressed(Key::R)) {
            resetCamera();
        }
        if (isKeyPressed(Key::F)) {
            flyToPointOfInterest();
        }
        if (isKeyPressed(Key::L)) {
            interactionLocked = not interactionLocked;
            setCameraPanEnabled(not interactionLocked);
            setCameraZoomEnabled(not interactionLocked);
        }

        background(rgba(18, 18, 24));

        beginCamera();
        drawWorld();
        endCamera();

        drawOverlay();
    }

    void flyToPointOfInterest()
    {
        const float2 screenCenter {
            static_cast<float>(getWindowSize().x) * 0.5f,
            static_cast<float>(getWindowSize().y) * 0.5f,
        };
        setCameraZoom(flyToZoom);
        setCameraPan(screenCenter.x - pointOfInterest.x * flyToZoom, screenCenter.y - pointOfInterest.y * flyToZoom);
    }

    void drawWorld()
    {
        noStroke();
        for (int gx = -gridExtent; gx <= gridExtent; ++gx) {
            for (int gy = -gridExtent; gy <= gridExtent; ++gy) {
                const bool checker = (gx + gy) % 2 == 0;
                fill(checker ? rgba(90, 140, 220) : rgba(70, 100, 160));
                circle(static_cast<float>(gx) * gridSpacing, static_cast<float>(gy) * gridSpacing, 14.0f);
            }
        }

        stroke(rgba(255, 210, 90));
        strokeWeight(2.0f);
        line(-40.0f, 0.0f, 40.0f, 0.0f);
        line(0.0f, -40.0f, 0.0f, 40.0f);

        noStroke();
        fill(rgba(255, 100, 140));
        circle(pointOfInterest.x, pointOfInterest.y, 24.0f);
    }

    void drawOverlay()
    {
        noStroke();
        fill(rgba(255));

        textAlign(TextAlignment::topLeft);
        textSize(14.0f);
        text("Drag with the middle mouse button to pan.", 16.0f, 16.0f);
        text("Scroll to zoom toward the cursor. Press R to reset the view.", 16.0f, 36.0f);
        text("Press F to fly to the pink dot. Press L to lock/unlock mouse control.", 16.0f, 56.0f);

        const float2 pan = getCameraPan();
        const float zoom = getCameraZoom();
        textAlign(TextAlignment::bottomLeft);
        text(std::format("zoom {:.2f}x   pan ({:.0f}, {:.0f})   {}", zoom, pan.x, pan.y, interactionLocked ? "LOCKED" : "unlocked"), 16.0f, static_cast<float>(getWindowSize().y) - 16.0f);
    }
};

SketchSpec p5::createSpec()
{
    return {
        .plugins = [] {
            std::vector<std::unique_ptr<Plugin>> plugins;
            plugins.push_back(camera::createCameraPlugin());
            return plugins;
        },
        .sketch = [] {
            return std::make_unique<Camera2DExample>();
        },
    };
}
