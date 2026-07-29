#include "history.hpp"
#include "layer.hpp"
#include "tools.hpp"
#include "ui.hpp"

#include <p5cpp/p5cpp.hpp>

#include <algorithm>
#include <string>

namespace
{
    using namespace paint;
    using namespace p5cpp;

    inline constexpr int WINDOW_WIDTH = 1400;
    inline constexpr int WINDOW_HEIGHT = 900;
    inline constexpr uint32_t CANVAS_WIDTH = 960;
    inline constexpr uint32_t CANVAS_HEIGHT = 640;

    struct PaintSketch : Sketch
    {
        Canvas canvas {CANVAS_WIDTH, CANVAS_HEIGHT};
        HistoryStack history;
        UiState ui;
        ToolState toolState;

        Tool tool = Tool::brush;
        color_t primaryColor = rgba(20, 20, 20);
        color_t secondaryColor = rgba(255, 255, 255);
        float brushSize = 10.0f;

        void setup() override
        {
            setWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
            setWindowTitle("p5cpp Paint");

            Font font = loadFont("example_assets/MapleMono-NF-Regular.ttf");
            textFont(font);
            initUi(ui, font);

            canvas.viewportX = TOOLBAR_WIDTH;
            canvas.viewportY = 40.0f;
            fitCanvasToViewport();
        }

        void fitCanvasToViewport()
        {
            const float viewportWidth = static_cast<float>(WINDOW_WIDTH) - TOOLBAR_WIDTH - PANEL_WIDTH;
            const float viewportHeight = static_cast<float>(WINDOW_HEIGHT) - canvas.viewportY - 20.0f;
            const float fitZoom = std::min(viewportWidth / static_cast<float>(canvas.getWidth()), viewportHeight / static_cast<float>(canvas.getHeight()));

            canvas.zoom = std::clamp(fitZoom, 0.1f, 1.0f);
            canvas.panX = (viewportWidth - static_cast<float>(canvas.getWidth()) * canvas.zoom) * 0.5f;
            canvas.panY = (viewportHeight - static_cast<float>(canvas.getHeight()) * canvas.zoom) * 0.5f;
        }

        // Commits/cancels whatever the previous tool was mid-doing, then
        // switches — used by both the keyboard-shortcut path here and the
        // toolbar-click path in draw() (see the `toolChanged` handling there).
        void setTool(Tool newTool)
        {
            if (newTool == tool) {
                return;
            }
            ToolContext ctx {canvas, history, tool, primaryColor, secondaryColor, brushSize, float2 {}, float2 {}, false};
            cancelToolInteraction(ctx, toolState);
            tool = newTool;
        }

        void clearActiveLayer()
        {
            discardFloatingSelection(toolState);
            history.push(canvas.captureState());
            pushCanvas(canvas.activeLayer().framebuffer);
                background(0, 0);
            popCanvas();
        }

        void event(const WindowEvent& windowEvent) override
        {
            if (windowEvent.type != EventType::keyPress) {
                return;
            }

            // The engine's FrameModule globally binds Space to toggle noLoop()/
            // loop() (see frame_module.cpp), independent of app state — typing a
            // space while composing text would otherwise freeze draw() entirely,
            // which looks exactly like a crash (frozen window). Counteract it
            // immediately: FrameModule::event() (which does the toggle) runs
            // before Sketch::event() in the same synchronous dispatch, so calling
            // loop() here undoes it within the same event before any frame is
            // skipped.
            if (windowEvent.keyEvent.key == Key::space and toolState.textComposing) {
                loop();
            }

            const bool ctrlHeld = (windowEvent.keyEvent.mods & KeyMod::ctrl) != 0;
            const bool shiftHeld = (windowEvent.keyEvent.mods & KeyMod::shift) != 0;

            if (ctrlHeld and windowEvent.keyEvent.key == Key::z) {
                // A floating selection's cut already mutated the canvas and was
                // snapshotted; undo/redo can land on a state that has nothing to
                // do with it anymore, so drop it here rather than risk pasting
                // stale pixels back later (see discardFloatingSelection()).
                discardFloatingSelection(toolState);
                if (shiftHeld) {
                    history.redo(canvas);
                } else {
                    history.undo(canvas);
                }
                return;
            }

            if (ctrlHeld and windowEvent.keyEvent.key == Key::y) {
                discardFloatingSelection(toolState);
                history.redo(canvas);
                return;
            }

            if (ctrlHeld and windowEvent.keyEvent.key == Key::s) {
                Framebuffer flattened = canvas.flattenToFramebuffer();
                saveImage("painting.png", flattened);
                unload(flattened);
                info("Paint: saved painting.png");
                return;
            }

            // Tool-switch shortcuts are suppressed while composing text, so
            // typing doesn't also change the active tool.
            if (toolState.textComposing) {
                return;
            }

            switch (windowEvent.keyEvent.key) {
                case Key::b: setTool(Tool::brush); break;
                case Key::e: setTool(Tool::eraser); break;
                case Key::l: setTool(Tool::line); break;
                case Key::r: setTool(Tool::rectangle); break;
                case Key::o: setTool(Tool::ellipse); break;
                case Key::g: setTool(Tool::fill); break;
                case Key::i: setTool(Tool::eyedropper); break;
                case Key::t: setTool(Tool::text); break;
                case Key::m: setTool(Tool::move); break;
                case Key::leftBracket: brushSize = std::max(1.0f, brushSize - 2.0f); break;
                case Key::rightBracket: brushSize = std::min(80.0f, brushSize + 2.0f); break;
                case Key::del: clearActiveLayer(); break;
                case Key::n:
                    history.push(canvas.captureState());
                    canvas.addLayer("Layer " + std::to_string(canvas.layers().size() + 1));
                    break;
                default: break;
            }
        }

        void draw() override
        {
            background(18, 18, 20);

            const float mouseX = static_cast<float>(getMouseX());
            const float mouseY = static_cast<float>(getMouseY());
            const bool overCanvasViewport = not isOverUiChrome(mouseX, WINDOW_WIDTH);

            // Zoom to cursor via the scroll wheel.
            const float scrollY = getScrollY();
            if (scrollY != 0.0f and overCanvasViewport) {
                const float2 beforeCanvas = canvas.screenToCanvas(mouseX, mouseY);
                canvas.zoom = std::clamp(canvas.zoom * (1.0f + scrollY * 0.1f), 0.1f, 8.0f);
                const float2 afterScreen = canvas.canvasToScreen(beforeCanvas.x, beforeCanvas.y);
                canvas.panX += mouseX - afterScreen.x;
                canvas.panY += mouseY - afterScreen.y;
            }

            // Middle-mouse-drag pans, leaving left/right free for tools.
            if (isMouseDragging(MouseButton::middle)) {
                canvas.panX += static_cast<float>(getMouseX() - getPMouseX());
                canvas.panY += static_cast<float>(getMouseY() - getPMouseY());
            }

            canvas.compositeToScreen();

            const float2 canvasMouse = canvas.screenToCanvas(mouseX, mouseY);
            const float2 canvasPMouse = canvas.screenToCanvas(static_cast<float>(getPMouseX()), static_cast<float>(getPMouseY()));

            canvas.pushCanvasTransform();
                ToolContext previewCtx {canvas, history, tool, primaryColor, secondaryColor, brushSize, canvasMouse, canvasPMouse, false};
                drawToolPreview(previewCtx, toolState);
            canvas.popCanvasTransform();

            UiContext uiCtx {tool, primaryColor, secondaryColor, brushSize, canvas, history};
            const bool toolChanged = updateUi(ui, uiCtx, WINDOW_WIDTH, WINDOW_HEIGHT);

            const bool overUI = not overCanvasViewport or ui.isDraggingAnyWidget();
            ToolContext toolCtx {canvas, history, tool, primaryColor, secondaryColor, brushSize, canvasMouse, canvasPMouse, overUI};

            if (toolChanged) {
                cancelToolInteraction(toolCtx, toolState);
            }

            updateTool(toolCtx, toolState);
        }
    };
} // namespace

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<PaintSketch>();
    }
} // namespace p5cpp
