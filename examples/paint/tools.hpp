#pragma once

#include "history.hpp"
#include "layer.hpp"

#include <p5cpp/p5cpp.hpp>

#include <string>

namespace paint
{
    enum class Tool
    {
        brush,
        eraser,
        line,
        rectangle,
        ellipse,
        fill,
        eyedropper,
        text,
        move,
    };

    const char* toolLabel(Tool tool);

    // Everything a tool needs for one frame. Built fresh each frame in main.cpp
    // from current input/UI state — primaryColor/secondaryColor are references
    // since the eyedropper tool writes back into them.
    struct ToolContext
    {
        Canvas& canvas;
        HistoryStack& history;

        Tool tool;
        p5cpp::color_t& primaryColor;
        p5cpp::color_t& secondaryColor;
        float brushSize;

        // Canvas-space mouse position (already run through canvas.screenToCanvas()).
        p5cpp::float2 canvasMouse;
        p5cpp::float2 canvasPMouse;

        // True while the mouse is over toolbar/panel chrome — tools ignore
        // press/drag/release while this is set, so clicking UI doesn't also
        // paint on the canvas underneath it.
        bool overUI;
    };

    // Per-tool state that must survive across frames while a multi-step
    // interaction (shape drag, text composition, marquee select/move) is in
    // progress. Owned by main.cpp's Sketch alongside the ToolContext.
    struct ToolState
    {
        bool shapeDragActive = false;
        p5cpp::MouseButton shapeButton = p5cpp::MouseButton::left;
        p5cpp::float2 shapeStartCanvas {};

        bool textComposing = false;
        p5cpp::float2 textAnchorCanvas {};
        std::string textBuffer;

        bool marqueeActive = false;   // still dragging out the initial selection rect
        p5cpp::float2 marqueeStartCanvas {};
        p5cpp::float2 marqueeEndCanvas {};

        bool hasSelection = false;    // a floating selection exists
        bool movingSelection = false; // currently dragging the floating selection
        p5cpp::int_rect selectionRect {};   // where the floating pixels were cut from
        p5cpp::float2 selectionOffset {};   // accumulated move offset, canvas units
        p5cpp::Pixels selectionPixels;      // the floating selection's own pixel buffer
        p5cpp::Texture selectionTexture;    // uploaded once per cut, for the live preview

        ~ToolState()
        {
            p5cpp::unload(selectionTexture);
        }
    };

    // Reads mouse press/drag/release for the active tool directly (isMouseDown/
    // isMousePressed/isMouseReleased/isMouseDragging) and mutates the canvas
    // accordingly, pushing history snapshots at the start of each committed
    // action. Call once per frame from draw().
    void updateTool(ToolContext& ctx, ToolState& state);

    // Draws whatever live (uncommitted) preview the active tool needs — must be
    // called within canvas.pushCanvasTransform()/popCanvasTransform() so it
    // lines up with the canvas content underneath.
    void drawToolPreview(const ToolContext& ctx, const ToolState& state);

    // Commits or discards any in-progress multi-step interaction. Call this
    // whenever the active tool is about to change, so switching tools mid-drag
    // doesn't leave a half-finished shape/selection/text dangling.
    void cancelToolInteraction(ToolContext& ctx, ToolState& state);

    // Drops a floating selection (if any) *without* pasting it back — unlike
    // cancelToolInteraction()/commitSelectionIfAny(), which paste. The
    // selection's cut already mutated the canvas and was snapshotted by
    // history; undo/redo can move the canvas to a state that has nothing to
    // do with that cut anymore, so pasting stale floating pixels back after
    // an undo/redo would silently corrupt it. Call this before history.undo()/
    // redo() instead.
    void discardFloatingSelection(ToolState& state);
} // namespace paint
