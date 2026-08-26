#include <p5cpp/p5cpp.hpp>

#include "building.hpp"
#include "camera.hpp"
#include "grid.hpp"

using namespace p5;
using namespace kingshot;

namespace
{
    // Number keys 1..3 select which placeable building the next left-click drops.
    // TownHall is intentionally excluded -- it's pre-placed and fixed.
    constexpr std::array<Key, 3> buildHotkeys {Key::Num1, Key::Num2, Key::Num3};
    constexpr std::array<BuildingType, 3> buildHotkeyTargets {BuildingType::Wall, BuildingType::Barracks, BuildingType::Farm};
}

struct KingshotClone : public Sketch
{
    Camera camera;
    BaseState baseState {Grid {.columns = 24, .rows = 24, .cellSize = 48.0f}};
    BuildingType selectedType = BuildingType::Wall;

    void setup() override
    {
        setWindowSize(1280, 720);
        setWindowTitle("Kingshot Clone");

        camera.position = gridWorldCenter(baseState.getGrid());
    }

    void event(const WindowEvent& windowEvent) override
    {
        if (windowEvent.is<WindowEvent::MouseScroll>()) {
            const auto& scroll = windowEvent.as<WindowEvent::MouseScroll>();
            const float zoomFactor = std::pow(1.1f, static_cast<float>(scroll.yOffset));
            zoomAtScreenPoint(camera, getMousePosition(), getScreenCenter(), zoomFactor);
        }
    }

    float2 getMousePosition() const
    {
        return float2 {.x = static_cast<float>(getMouseX()), .y = static_cast<float>(getMouseY())};
    }

    float2 getScreenCenter() const
    {
        // Window size (not getWidth()/getHeight(), which read the framebuffer and are
        // only valid while one is pushed during draw()) -- also keeps this in the same
        // coordinate space as getMouseX/Y(), which report window coordinates.
        const uint2 windowSize = getWindowSize();
        return float2 {.x = static_cast<float>(windowSize.x) * 0.5f, .y = static_cast<float>(windowSize.y) * 0.5f};
    }

    void handleInput()
    {
        for (size_t i = 0; i < buildHotkeys.size(); ++i) {
            if (isKeyPressed(buildHotkeys[i])) {
                selectedType = buildHotkeyTargets[i];
            }
        }

        // Right-mouse drag pans the camera; left-click drops the selected building.
        if (isMouseButtonDown(MouseButton::Right)) {
            panByScreenDelta(camera, float2 {.x = static_cast<float>(getMouseDeltaX()), .y = static_cast<float>(getMouseDeltaY())});
        }

        if (isMouseButtonPressed(MouseButton::Left)) {
            const float2 worldPos = screenToWorld(camera, getMousePosition(), getScreenCenter());
            const int2 cell = worldToCell(baseState.getGrid(), worldPos);
            baseState.tryPlace(selectedType, cell);
        }
    }

    void drawHud() const
    {
        fill(rgba(255));
        noStroke();
        textSize(16.0f);

        for (size_t i = 0; i < buildHotkeys.size(); ++i) {
            const BuildingDef& def = getBuildingDef(buildHotkeyTargets[i]);
            const bool isSelected = def.type == selectedType;
            fill(isSelected ? rgba(255, 220, 120) : rgba(220));
            text(std::format("[{}] {}", i + 1, def.name), 16.0f, 16.0f + static_cast<float>(i) * 22.0f);
        }
    }

    void draw() override
    {
        background(rgba(30, 32, 40));

        const float2 mouseScreen = getMousePosition();
        const bool cursorInWindow = isCursorInWindow();

        handleInput();

        withMatrix([&]() {
            applyCameraTransform(camera, getScreenCenter());

            std::optional<int2> hoveredCell;
            if (cursorInWindow) {
                const float2 worldPos = screenToWorld(camera, mouseScreen, getScreenCenter());
                hoveredCell = worldToCell(baseState.getGrid(), worldPos);
            }

            baseState.draw(hoveredCell, selectedType);
        });

        drawHud();
    }
};

SketchSpec p5::createSpec()
{
    return {
        .sketch = [] {
            return std::make_unique<KingshotClone>();
        },
    };
}
