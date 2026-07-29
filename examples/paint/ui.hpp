#pragma once

#include "history.hpp"
#include "layer.hpp"
#include "tools.hpp"

#include <p5cpp/p5cpp.hpp>

namespace paint
{
    inline constexpr float TOOLBAR_WIDTH = 170.0f;
    inline constexpr float PANEL_WIDTH = 240.0f;

    // Everything the UI reads/writes for one frame. Built fresh each frame in
    // main.cpp, mirroring ToolContext.
    struct UiContext
    {
        Tool& tool;
        p5cpp::color_t& primaryColor;
        p5cpp::color_t& secondaryColor;
        float& brushSize;
        Canvas& canvas;
        HistoryStack& history;
    };

    // Persistent UI state, mirroring ToolState: the color-picker's own
    // hue/sat/value (kept in sync with UiContext::primaryColor, see ui.cpp),
    // cached gradient textures that only need rebuilding when the hue changes,
    // and which widget (if any) is currently being dragged.
    struct UiState
    {
        p5cpp::Font font;

        float hue = 0.0f;
        float saturation = 0.0f;
        float value = 1.0f;
        p5cpp::color_t lastSyncedPrimary = 0;

        p5cpp::Texture hueStripTexture;
        p5cpp::Texture svSquareTexture;
        float svSquareHue = -1.0f; // hue the cached svSquareTexture was built for

        bool draggingHue = false;
        bool draggingSv = false;
        bool draggingBrushSlider = false;
        bool draggingOpacity = false;

        // True while any widget above is being dragged, even if the drag has
        // since moved past the panel's edge — main.cpp ORs this into overUI so
        // a slider drag that spills outside the panel rect doesn't also start
        // painting on the canvas underneath it.
        bool isDraggingAnyWidget() const { return draggingHue or draggingSv or draggingBrushSlider or draggingOpacity; }

        ~UiState()
        {
            p5cpp::unload(hueStripTexture);
            p5cpp::unload(svSquareTexture);
        }
    };

    // Builds the (hue-independent) hue-strip gradient texture once.
    void initUi(UiState& ui, p5cpp::Font font);

    // True if (screenX, screenY) is over the toolbar or the right-hand panel —
    // main.cpp uses this to build ToolContext::overUI so canvas tools ignore
    // input that's actually meant for the UI.
    bool isOverUiChrome(float screenX, int windowWidth);

    // Draws the toolbar (left) and color-picker/brush/layers panel (right), and
    // handles their own input (button clicks, slider/swatch drags). Returns
    // true if the active tool changed this frame, so main.cpp knows to call
    // cancelToolInteraction() for whatever the previous tool was doing before
    // dispatching input to the new one.
    bool updateUi(UiState& ui, UiContext& ctx, int windowWidth, int windowHeight);
} // namespace paint
