#include "tools.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace p5cpp;

namespace paint
{
    const char* toolLabel(Tool tool)
    {
        switch (tool) {
            case Tool::brush: return "Brush (B)";
            case Tool::eraser: return "Eraser (E)";
            case Tool::line: return "Line (L)";
            case Tool::rectangle: return "Rect (R)";
            case Tool::ellipse: return "Ellipse (O)";
            case Tool::fill: return "Fill (G)";
            case Tool::eyedropper: return "Eyedropper (I)";
            case Tool::text: return "Text (T)";
            case Tool::move: return "Move (M)";
        }
        return "?";
    }
} // namespace paint

namespace
{
    using namespace paint;

    // Fixed font size for the text tool, independent of whatever textSize() the
    // UI panel's own labels last left the global render state at.
    constexpr float kTextToolSize = 32.0f;

    color_t colorForButton(const ToolContext& ctx, MouseButton button)
    {
        return button == MouseButton::left ? ctx.primaryColor : ctx.secondaryColor;
    }

    void appendUtf8(std::string& out, char32_t codepoint)
    {
        if (codepoint <= 0x7F) {
            out += static_cast<char>(codepoint);
        } else if (codepoint <= 0x7FF) {
            out += static_cast<char>(0xC0 | (codepoint >> 6));
            out += static_cast<char>(0x80 | (codepoint & 0x3F));
        } else if (codepoint <= 0xFFFF) {
            out += static_cast<char>(0xE0 | (codepoint >> 12));
            out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (codepoint & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (codepoint >> 18));
            out += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
    }

    void popLastUtf8Char(std::string& s)
    {
        if (s.empty()) {
            return;
        }
        s.pop_back();
        while (not s.empty() and (static_cast<unsigned char>(s.back()) & 0xC0) == 0x80) {
            s.pop_back();
        }
    }

    void strokeSegment(Canvas& canvas, color_t color, float brushSize, float2 from, float2 to, bool erase)
    {
        pushCanvas(canvas.activeLayer().framebuffer);
            if (erase) {
                blendMode(BlendMode::none);
                stroke(rgba(0, 0, 0, 0));
            } else {
                blendMode(BlendMode::alpha);
                stroke(color);
            }
            strokeWeight(brushSize);
            strokeCap(StrokeCap::round);
            strokeJoin(StrokeJoin::round);
            noFill();
            line(from.x, from.y, to.x, to.y);
            blendMode(BlendMode::alpha);
        popCanvas();
    }

    void updateBrushOrEraser(ToolContext& ctx, ToolState& state, bool erase)
    {
        for (MouseButton button : {MouseButton::left, MouseButton::right}) {
            if (ctx.overUI) {
                continue;
            }
            if (isMousePressed(button)) {
                ctx.history.push(ctx.canvas.captureState());
                strokeSegment(ctx.canvas, colorForButton(ctx, button), ctx.brushSize, ctx.canvasMouse, ctx.canvasMouse, erase);
            }
            if (isMouseDragging(button)) {
                strokeSegment(ctx.canvas, colorForButton(ctx, button), ctx.brushSize, ctx.canvasPMouse, ctx.canvasMouse, erase);
            }
        }
    }

    void drawShapeGeometry(Tool tool, float2 a, float2 b)
    {
        switch (tool) {
            case Tool::line:
                line(a.x, a.y, b.x, b.y);
                break;
            case Tool::rectangle: {
                const float left = std::min(a.x, b.x);
                const float top = std::min(a.y, b.y);
                rect(left, top, std::abs(b.x - a.x), std::abs(b.y - a.y));
                break;
            }
            case Tool::ellipse: {
                const float centerX = (a.x + b.x) * 0.5f;
                const float centerY = (a.y + b.y) * 0.5f;
                ellipse(centerX, centerY, std::abs(b.x - a.x) * 0.5f, std::abs(b.y - a.y) * 0.5f);
                break;
            }
            default:
                break;
        }
    }

    void commitShape(ToolContext& ctx, ToolState& state, Tool tool)
    {
        ctx.history.push(ctx.canvas.captureState());

        pushCanvas(ctx.canvas.activeLayer().framebuffer);
            blendMode(BlendMode::alpha);
            stroke(colorForButton(ctx, state.shapeButton));
            strokeWeight(ctx.brushSize);
            strokeCap(StrokeCap::round);
            strokeJoin(StrokeJoin::round);
            noFill();
            drawShapeGeometry(tool, state.shapeStartCanvas, ctx.canvasMouse);
        popCanvas();
    }

    void updateShapeTool(ToolContext& ctx, ToolState& state, Tool tool)
    {
        if (not state.shapeDragActive and not ctx.overUI) {
            for (MouseButton button : {MouseButton::left, MouseButton::right}) {
                if (isMousePressed(button)) {
                    state.shapeDragActive = true;
                    state.shapeButton = button;
                    state.shapeStartCanvas = ctx.canvasMouse;
                    break;
                }
            }
        }

        if (state.shapeDragActive and isMouseReleased(state.shapeButton)) {
            commitShape(ctx, state, tool);
            state.shapeDragActive = false;
        }
    }

    void previewShape(const ToolContext& ctx, const ToolState& state)
    {
        noFill();
        stroke(withAlpha(colorForButton(ctx, state.shapeButton), 160));
        strokeWeight(ctx.brushSize);
        strokeCap(StrokeCap::round);
        strokeJoin(StrokeJoin::round);
        drawShapeGeometry(ctx.tool, state.shapeStartCanvas, ctx.canvasMouse);
    }

    void updateFillTool(ToolContext& ctx)
    {
        if (ctx.overUI) {
            return;
        }

        for (MouseButton button : {MouseButton::left, MouseButton::right}) {
            if (not isMousePressed(button)) {
                continue;
            }

            const int x = static_cast<int>(ctx.canvasMouse.x);
            const int y = static_cast<int>(ctx.canvasMouse.y);
            if (x < 0 or y < 0 or static_cast<uint32_t>(x) >= ctx.canvas.getWidth() or static_cast<uint32_t>(y) >= ctx.canvas.getHeight()) {
                continue;
            }

            ctx.history.push(ctx.canvas.captureState());

            pushCanvas(ctx.canvas.activeLayer().framebuffer);
                Pixels pixels = loadPixels();
                floodFill(pixels, x, y, colorForButton(ctx, button));
                updatePixels(pixels);
            popCanvas();
        }
    }

    void updateEyedropperTool(ToolContext& ctx)
    {
        if (ctx.overUI) {
            return;
        }

        const bool leftClick = isMousePressed(MouseButton::left);
        const bool rightClick = isMousePressed(MouseButton::right);
        if (not leftClick and not rightClick) {
            return;
        }

        const int x = static_cast<int>(ctx.canvasMouse.x);
        const int y = static_cast<int>(ctx.canvasMouse.y);
        if (x < 0 or y < 0 or static_cast<uint32_t>(x) >= ctx.canvas.getWidth() or static_cast<uint32_t>(y) >= ctx.canvas.getHeight()) {
            return;
        }

        pushCanvas(ctx.canvas.activeLayer().framebuffer);
            Pixels pixels = loadPixels();
        popCanvas();

        const color_t sampled = pixels.get(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
        if (leftClick) {
            ctx.primaryColor = sampled;
        } else {
            ctx.secondaryColor = sampled;
        }
    }

    // text() is rendered with standard non-premultiplied "src-over" alpha
    // blending (BlendMode::alpha), which is correct when compositing onto an
    // opaque destination (that's why the live preview, drawn over the opaque
    // window background, looks fine) but produces a visible gray/dark halo on
    // anti-aliased glyph edges when composited directly onto a
    // transparent-background layer: the blend factors mix the edge color
    // toward the destination's RGB even where the destination's alpha is 0
    // and its RGB is meant to be irrelevant. This is a well-known limitation
    // of straight-alpha compositing shared by many simple 2D APIs (including
    // the browser Canvas2D underneath p5.js itself), not something fixable by
    // changing the blend *mode* — it needs actual premultiplied-alpha math.
    //
    // Fixed here by routing around the GPU blend entirely: render opaque
    // white-on-black into a small scratch Framebuffer (blending correctly,
    // exactly like the preview), read the luminance back as a coverage mask,
    // and alpha-composite it onto the layer ourselves with the correct
    // Porter-Duff "over" formula.
    void compositeTextOntoLayer(Canvas& canvas, const std::string& text_, float2 anchor, color_t fillColor)
    {
        textAlign(TextAlign::topLeft);
        textSize(kTextToolSize);
        const TextLayout layout = textLayout(text_, anchor.x, anchor.y);

        constexpr float margin = 8.0f;
        const int scratchWidth = static_cast<int>(std::ceil(layout.bounds.width)) + static_cast<int>(margin * 2.0f);
        const int scratchHeight = static_cast<int>(std::ceil(layout.bounds.height)) + static_cast<int>(margin * 2.0f);
        if (scratchWidth <= 0 or scratchHeight <= 0) {
            return;
        }

        const float originX = layout.bounds.left - margin;
        const float originY = layout.bounds.top - margin;

        Framebuffer scratch = createFramebuffer(static_cast<uint32_t>(scratchWidth), static_cast<uint32_t>(scratchHeight));
        pushCanvas(scratch);
            blendMode(BlendMode::alpha);
            background(0); // opaque black
            fill(255);      // opaque white
            noStroke();
            textAlign(TextAlign::topLeft);
            textSize(kTextToolSize);
            text(text_, anchor.x - originX, anchor.y - originY);
        popCanvas();

        pushCanvas(scratch);
            const Pixels coverage = loadPixels();
        popCanvas();

        const int fillR = red(fillColor);
        const int fillG = green(fillColor);
        const int fillB = blue(fillColor);
        const float fillAlpha01 = static_cast<float>(alpha(fillColor)) / 255.0f;

        pushCanvas(canvas.activeLayer().framebuffer);
            Pixels layerPixels = loadPixels();
            const uint32_t layerWidth = layerPixels.getWidth();
            const uint32_t layerHeight = layerPixels.getHeight();

            for (int sy = 0; sy < scratchHeight; ++sy) {
                const int ty = static_cast<int>(originY) + sy;
                if (ty < 0 or static_cast<uint32_t>(ty) >= layerHeight) {
                    continue;
                }
                for (int sx = 0; sx < scratchWidth; ++sx) {
                    const int tx = static_cast<int>(originX) + sx;
                    if (tx < 0 or static_cast<uint32_t>(tx) >= layerWidth) {
                        continue;
                    }

                    const float srcA = (static_cast<float>(red(coverage.get(static_cast<uint32_t>(sx), static_cast<uint32_t>(sy)))) / 255.0f) * fillAlpha01;
                    if (srcA <= 0.0f) {
                        continue;
                    }

                    const color_t dst = layerPixels.get(static_cast<uint32_t>(tx), static_cast<uint32_t>(ty));
                    const float dstA = static_cast<float>(alpha(dst)) / 255.0f;
                    const float outA = srcA + dstA * (1.0f - srcA);

                    if (outA <= 0.0001f) {
                        layerPixels.set(static_cast<uint32_t>(tx), static_cast<uint32_t>(ty), rgba(0, 0, 0, 0));
                        continue;
                    }

                    const float outR = (static_cast<float>(fillR) * srcA + static_cast<float>(red(dst)) * dstA * (1.0f - srcA)) / outA;
                    const float outG = (static_cast<float>(fillG) * srcA + static_cast<float>(green(dst)) * dstA * (1.0f - srcA)) / outA;
                    const float outB = (static_cast<float>(fillB) * srcA + static_cast<float>(blue(dst)) * dstA * (1.0f - srcA)) / outA;

                    layerPixels.set(static_cast<uint32_t>(tx), static_cast<uint32_t>(ty),
                                     rgba(static_cast<int>(std::clamp(outR, 0.0f, 255.0f)),
                                          static_cast<int>(std::clamp(outG, 0.0f, 255.0f)),
                                          static_cast<int>(std::clamp(outB, 0.0f, 255.0f)),
                                          static_cast<int>(std::clamp(outA * 255.0f, 0.0f, 255.0f))));
                }
            }

            updatePixels(layerPixels);
        popCanvas();
    }

    void commitTextIfAny(ToolContext& ctx, ToolState& state)
    {
        if (not state.textComposing) {
            return;
        }
        state.textComposing = false;

        if (state.textBuffer.empty()) {
            return;
        }

        ctx.history.push(ctx.canvas.captureState());
        compositeTextOntoLayer(ctx.canvas, state.textBuffer, state.textAnchorCanvas, ctx.primaryColor);
    }

    void updateTextTool(ToolContext& ctx, ToolState& state)
    {
        if (isMousePressed(MouseButton::left) and not ctx.overUI) {
            commitTextIfAny(ctx, state); // finish whatever was being composed at the old anchor first
            state.textComposing = true;
            state.textAnchorCanvas = ctx.canvasMouse;
            state.textBuffer.clear();
        }

        if (not state.textComposing) {
            return;
        }

        for (char32_t codepoint : getCharsTyped()) {
            appendUtf8(state.textBuffer, codepoint);
        }

        if (isKeyPressed(Key::backspace)) {
            popLastUtf8Char(state.textBuffer);
        }

        if (isKeyPressed(Key::enter)) {
            commitTextIfAny(ctx, state);
        }
    }

    void previewText(const ToolContext& ctx, const ToolState& state)
    {
        fill(withAlpha(ctx.primaryColor, 200));
        noStroke();
        textAlign(TextAlign::topLeft);
        textSize(kTextToolSize);
        const std::string preview = state.textBuffer + "|";
        text(preview, state.textAnchorCanvas.x, state.textAnchorCanvas.y);
    }

    bool pointInFloatingSelection(const ToolState& state, float2 p)
    {
        if (not state.hasSelection) {
            return false;
        }
        const float left = static_cast<float>(state.selectionRect.left) + state.selectionOffset.x;
        const float top = static_cast<float>(state.selectionRect.top) + state.selectionOffset.y;
        const float right = left + static_cast<float>(state.selectionRect.width);
        const float bottom = top + static_cast<float>(state.selectionRect.height);
        return p.x >= left and p.x <= right and p.y >= top and p.y <= bottom;
    }

    void resetFloatingSelectionState(ToolState& state)
    {
        state.hasSelection = false;
        state.movingSelection = false;
        unload(state.selectionTexture);
        state.selectionPixels = Pixels();
    }

    void commitSelectionIfAny(ToolContext& ctx, ToolState& state)
    {
        if (not state.hasSelection) {
            return;
        }

        ctx.history.push(ctx.canvas.captureState());

        const int destX = state.selectionRect.left + static_cast<int>(std::lround(state.selectionOffset.x));
        const int destY = state.selectionRect.top + static_cast<int>(std::lround(state.selectionOffset.y));

        pushCanvas(ctx.canvas.activeLayer().framebuffer);
            Pixels layerPixels = loadPixels();
            const uint32_t layerWidth = layerPixels.getWidth();
            const uint32_t layerHeight = layerPixels.getHeight();

            for (int sy = 0; sy < state.selectionRect.height; ++sy) {
                const int ty = destY + sy;
                if (ty < 0 or static_cast<uint32_t>(ty) >= layerHeight) {
                    continue;
                }
                for (int sx = 0; sx < state.selectionRect.width; ++sx) {
                    const int tx = destX + sx;
                    if (tx < 0 or static_cast<uint32_t>(tx) >= layerWidth) {
                        continue;
                    }
                    layerPixels.set(static_cast<uint32_t>(tx), static_cast<uint32_t>(ty),
                                     state.selectionPixels.get(static_cast<uint32_t>(sx), static_cast<uint32_t>(sy)));
                }
            }

            updatePixels(layerPixels);
        popCanvas();

        resetFloatingSelectionState(state);
    }

    void finalizeMarquee(ToolContext& ctx, ToolState& state)
    {
        const float x0 = std::min(state.marqueeStartCanvas.x, state.marqueeEndCanvas.x);
        const float y0 = std::min(state.marqueeStartCanvas.y, state.marqueeEndCanvas.y);
        const float x1 = std::max(state.marqueeStartCanvas.x, state.marqueeEndCanvas.x);
        const float y1 = std::max(state.marqueeStartCanvas.y, state.marqueeEndCanvas.y);

        const int left = std::clamp(static_cast<int>(x0), 0, static_cast<int>(ctx.canvas.getWidth()));
        const int top = std::clamp(static_cast<int>(y0), 0, static_cast<int>(ctx.canvas.getHeight()));
        const int right = std::clamp(static_cast<int>(x1), 0, static_cast<int>(ctx.canvas.getWidth()));
        const int bottom = std::clamp(static_cast<int>(y1), 0, static_cast<int>(ctx.canvas.getHeight()));

        const int width = right - left;
        const int height = bottom - top;
        if (width <= 0 or height <= 0) {
            return; // degenerate marquee (a click with no drag) — nothing to select
        }

        ctx.history.push(ctx.canvas.captureState());

        Pixels cut(static_cast<uint32_t>(width), static_cast<uint32_t>(height));

        pushCanvas(ctx.canvas.activeLayer().framebuffer);
            Pixels layerPixels = loadPixels();
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    const color_t c = layerPixels.get(static_cast<uint32_t>(left + x), static_cast<uint32_t>(top + y));
                    cut.set(static_cast<uint32_t>(x), static_cast<uint32_t>(y), c);
                    layerPixels.set(static_cast<uint32_t>(left + x), static_cast<uint32_t>(top + y), rgba(0, 0, 0, 0));
                }
            }
            updatePixels(layerPixels);
        popCanvas();

        state.selectionRect = int_rect {left, top, width, height};
        state.selectionOffset = float2 {0.0f, 0.0f};
        unload(state.selectionTexture); // in case a previous selection wasn't reset/committed
        state.selectionTexture = loadTexture(static_cast<uint32_t>(width), static_cast<uint32_t>(height), flippedRows(cut).data());
        state.selectionPixels = std::move(cut);
        state.hasSelection = true;
    }

    void updateMoveTool(ToolContext& ctx, ToolState& state)
    {
        if (not state.marqueeActive and not state.movingSelection and not ctx.overUI and isMousePressed(MouseButton::left)) {
            if (state.hasSelection and pointInFloatingSelection(state, ctx.canvasMouse)) {
                state.movingSelection = true;
            } else {
                commitSelectionIfAny(ctx, state);
                state.marqueeActive = true;
                state.marqueeStartCanvas = ctx.canvasMouse;
                state.marqueeEndCanvas = ctx.canvasMouse;
            }
        }

        if (state.marqueeActive) {
            if (isMouseDragging(MouseButton::left)) {
                state.marqueeEndCanvas = ctx.canvasMouse;
            }
            if (isMouseReleased(MouseButton::left)) {
                state.marqueeActive = false;
                finalizeMarquee(ctx, state);
            }
        }

        if (state.movingSelection) {
            if (isMouseDragging(MouseButton::left)) {
                state.selectionOffset.x += ctx.canvasMouse.x - ctx.canvasPMouse.x;
                state.selectionOffset.y += ctx.canvasMouse.y - ctx.canvasPMouse.y;
            }
            if (isMouseReleased(MouseButton::left)) {
                state.movingSelection = false;
            }
        }
    }

    void previewMove(const ToolContext& ctx, const ToolState& state)
    {
        const float outlineWeight = 1.5f / std::max(ctx.canvas.zoom, 0.001f);

        if (state.marqueeActive) {
            noFill();
            stroke(255);
            strokeWeight(outlineWeight);
            const float x0 = std::min(state.marqueeStartCanvas.x, state.marqueeEndCanvas.x);
            const float y0 = std::min(state.marqueeStartCanvas.y, state.marqueeEndCanvas.y);
            rect(x0, y0, std::abs(state.marqueeEndCanvas.x - state.marqueeStartCanvas.x), std::abs(state.marqueeEndCanvas.y - state.marqueeStartCanvas.y));
        }

        if (state.hasSelection) {
            const float left = static_cast<float>(state.selectionRect.left) + state.selectionOffset.x;
            const float top = static_cast<float>(state.selectionRect.top) + state.selectionOffset.y;
            const float width = static_cast<float>(state.selectionRect.width);
            const float height = static_cast<float>(state.selectionRect.height);

            noTint();
            image(state.selectionTexture, left, top, width, height);

            noFill();
            stroke(255);
            strokeWeight(outlineWeight);
            rect(left, top, width, height);
        }
    }
} // namespace

namespace paint
{
    void updateTool(ToolContext& ctx, ToolState& state)
    {
        switch (ctx.tool) {
            case Tool::brush: updateBrushOrEraser(ctx, state, false); break;
            case Tool::eraser: updateBrushOrEraser(ctx, state, true); break;
            case Tool::line: updateShapeTool(ctx, state, Tool::line); break;
            case Tool::rectangle: updateShapeTool(ctx, state, Tool::rectangle); break;
            case Tool::ellipse: updateShapeTool(ctx, state, Tool::ellipse); break;
            case Tool::fill: updateFillTool(ctx); break;
            case Tool::eyedropper: updateEyedropperTool(ctx); break;
            case Tool::text: updateTextTool(ctx, state); break;
            case Tool::move: updateMoveTool(ctx, state); break;
        }
    }

    void drawToolPreview(const ToolContext& ctx, const ToolState& state)
    {
        switch (ctx.tool) {
            case Tool::line:
            case Tool::rectangle:
            case Tool::ellipse:
                if (state.shapeDragActive) {
                    previewShape(ctx, state);
                }
                break;
            case Tool::text:
                if (state.textComposing) {
                    previewText(ctx, state);
                }
                break;
            case Tool::move:
                previewMove(ctx, state);
                break;
            default:
                break;
        }
    }

    void cancelToolInteraction(ToolContext& ctx, ToolState& state)
    {
        state.shapeDragActive = false;
        state.marqueeActive = false;
        state.movingSelection = false;

        commitTextIfAny(ctx, state);
        commitSelectionIfAny(ctx, state);
    }

    void discardFloatingSelection(ToolState& state)
    {
        resetFloatingSelectionState(state);
    }
} // namespace paint
