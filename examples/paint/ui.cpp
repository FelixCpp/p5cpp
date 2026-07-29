#include "ui.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <string>
#include <vector>

using namespace p5cpp;

namespace
{
    using namespace paint;

    void rgbToHsv(color_t c, float& h, float& s, float& v)
    {
        const float r = static_cast<float>(red(c)) / 255.0f;
        const float g = static_cast<float>(green(c)) / 255.0f;
        const float b = static_cast<float>(blue(c)) / 255.0f;

        const float maxC = std::max({r, g, b});
        const float minC = std::min({r, g, b});
        const float delta = maxC - minC;

        v = maxC;
        s = maxC <= 0.0001f ? 0.0f : delta / maxC;

        if (delta <= 0.0001f) {
            h = 0.0f;
        } else if (maxC == r) {
            h = 60.0f * std::fmod((g - b) / delta, 6.0f);
        } else if (maxC == g) {
            h = 60.0f * ((b - r) / delta + 2.0f);
        } else {
            h = 60.0f * ((r - g) / delta + 4.0f);
        }
        if (h < 0.0f) {
            h += 360.0f;
        }
    }

    void rebuildHueStrip(UiState& ui)
    {
        constexpr uint32_t width = 256;
        Pixels pixels(width, 1);
        for (uint32_t x = 0; x < width; ++x) {
            const float hue = static_cast<float>(x) / static_cast<float>(width - 1) * 360.0f;
            pixels.set(x, 0, hsv(hue, 1.0f, 1.0f));
        }
        ui.hueStripTexture = loadTexture(width, 1, flippedRows(pixels).data());
    }

    void rebuildSvSquare(UiState& ui)
    {
        constexpr uint32_t size = 48;
        Pixels pixels(size, size);
        for (uint32_t y = 0; y < size; ++y) {
            const float value = 1.0f - static_cast<float>(y) / static_cast<float>(size - 1);
            for (uint32_t x = 0; x < size; ++x) {
                const float saturation = static_cast<float>(x) / static_cast<float>(size - 1);
                pixels.set(x, y, hsv(ui.hue, saturation, value));
            }
        }
        unload(ui.svSquareTexture);
        ui.svSquareTexture = loadTexture(size, size, flippedRows(pixels).data());
        ui.svSquareHue = ui.hue;
    }

    void drawButton(float_rect r, const std::string& label, bool active, float mouseX, float mouseY)
    {
        const bool hovering = r.contains(mouseX, mouseY);
        noStroke();
        fill(active ? rgba(90, 140, 220) : (hovering ? rgba(72, 72, 78) : rgba(56, 56, 60)));
        rect(r.left, r.top, r.width, r.height, BorderRadius::circular(6.0f));

        fill(255);
        textAlign(TextAlign::center);
        textSize(14.0f);
        text(label, r.left + r.width * 0.5f, r.top + r.height * 0.5f);
    }

    bool drawToolbar(UiContext& ctx, int windowHeight)
    {
        static constexpr Tool kTools[] = {
            Tool::brush, Tool::eraser, Tool::line, Tool::rectangle, Tool::ellipse,
            Tool::fill, Tool::eyedropper, Tool::text, Tool::move,
        };

        const float mouseX = static_cast<float>(getMouseX());
        const float mouseY = static_cast<float>(getMouseY());

        noStroke();
        fill(32, 32, 36);
        rect(0.0f, 0.0f, TOOLBAR_WIDTH, static_cast<float>(windowHeight));

        bool changed = false;
        float y = 20.0f;
        for (Tool tool : kTools) {
            const float_rect r {10.0f, y, TOOLBAR_WIDTH - 20.0f, 42.0f};
            drawButton(r, toolLabel(tool), tool == ctx.tool, mouseX, mouseY);

            if (r.contains(mouseX, mouseY) and isMousePressed(MouseButton::left) and tool != ctx.tool) {
                ctx.tool = tool;
                changed = true;
            }

            y += 50.0f;
        }

        return changed;
    }

    void drawColorPicker(UiState& ui, UiContext& ctx, float panelX, float contentWidth, float mouseX, float mouseY)
    {
        fill(255);
        textAlign(TextAlign::topLeft);
        textSize(15.0f);
        text("Color", panelX, 14.0f);

        const float_rect hueRect {panelX, 45.0f, contentWidth, 20.0f};
        noTint();
        image(ui.hueStripTexture, hueRect.left, hueRect.top, hueRect.width, hueRect.height);

        if (hueRect.contains(mouseX, mouseY) and isMousePressed(MouseButton::left)) {
            ui.draggingHue = true;
        }
        if (ui.draggingHue) {
            if (isMouseDown(MouseButton::left)) {
                const float t = std::clamp((mouseX - hueRect.left) / hueRect.width, 0.0f, 1.0f);
                ui.hue = t * 360.0f;
            } else {
                ui.draggingHue = false;
            }
        }

        {
            const float markerX = hueRect.left + (ui.hue / 360.0f) * hueRect.width;
            noFill();
            stroke(255);
            strokeWeight(2.0f);
            line(markerX, hueRect.top - 2.0f, markerX, hueRect.top + hueRect.height + 2.0f);
        }

        if (ui.svSquareHue != ui.hue) {
            rebuildSvSquare(ui);
        }

        const float_rect svRect {panelX, 75.0f, contentWidth, 150.0f};
        noTint();
        image(ui.svSquareTexture, svRect.left, svRect.top, svRect.width, svRect.height);

        if (svRect.contains(mouseX, mouseY) and isMousePressed(MouseButton::left)) {
            ui.draggingSv = true;
        }
        if (ui.draggingSv) {
            if (isMouseDown(MouseButton::left)) {
                ui.saturation = std::clamp((mouseX - svRect.left) / svRect.width, 0.0f, 1.0f);
                ui.value = 1.0f - std::clamp((mouseY - svRect.top) / svRect.height, 0.0f, 1.0f);
            } else {
                ui.draggingSv = false;
            }
        }

        {
            const float markerX = svRect.left + ui.saturation * svRect.width;
            const float markerY = svRect.top + (1.0f - ui.value) * svRect.height;
            noFill();
            stroke(ui.value > 0.6f ? rgba(0, 0, 0) : rgba(255, 255, 255));
            strokeWeight(2.0f);
            circle(markerX, markerY, 8.0f);
        }

        ctx.primaryColor = hsv(ui.hue, ui.saturation, ui.value);

        const float swatchY = 240.0f;
        const float_rect secondarySwatch {panelX + 18.0f, swatchY + 14.0f, 36.0f, 36.0f};
        const float_rect primarySwatch {panelX, swatchY, 36.0f, 36.0f};
        const float_rect swapButton {panelX + 72.0f, swatchY + 8.0f, 34.0f, 34.0f};

        noStroke();
        fill(ctx.secondaryColor);
        rect(secondarySwatch.left, secondarySwatch.top, secondarySwatch.width, secondarySwatch.height);
        fill(ctx.primaryColor);
        rect(primarySwatch.left, primarySwatch.top, primarySwatch.width, primarySwatch.height);
        noFill();
        stroke(255);
        strokeWeight(1.0f);
        rect(primarySwatch.left, primarySwatch.top, primarySwatch.width, primarySwatch.height);

        drawButton(swapButton, "<>", false, mouseX, mouseY);
        if (swapButton.contains(mouseX, mouseY) and isMousePressed(MouseButton::left)) {
            std::swap(ctx.primaryColor, ctx.secondaryColor);
            rgbToHsv(ctx.primaryColor, ui.hue, ui.saturation, ui.value);
        }

        static constexpr color_t kPresets[] = {
            rgba(0, 0, 0), rgba(255, 255, 255), rgba(220, 50, 50), rgba(240, 150, 30),
            rgba(240, 220, 40), rgba(60, 180, 70), rgba(40, 140, 220), rgba(120, 70, 200),
            rgba(230, 100, 170), rgba(120, 90, 60),
        };
        const float presetTop = 295.0f;
        const float presetSize = 18.0f;
        const float presetGap = 4.0f;
        const int presetsPerRow = 5;
        for (size_t i = 0; i < std::size(kPresets); ++i) {
            const int row = static_cast<int>(i) / presetsPerRow;
            const int col = static_cast<int>(i) % presetsPerRow;
            const float_rect r {
                panelX + static_cast<float>(col) * (presetSize + presetGap),
                presetTop + static_cast<float>(row) * (presetSize + presetGap),
                presetSize, presetSize,
            };
            noStroke();
            fill(kPresets[i]);
            rect(r.left, r.top, r.width, r.height);

            if (r.contains(mouseX, mouseY) and isMousePressed(MouseButton::left)) {
                ctx.primaryColor = kPresets[i];
                rgbToHsv(ctx.primaryColor, ui.hue, ui.saturation, ui.value);
            }
        }
    }

    void drawBrushSlider(UiState& ui, UiContext& ctx, float panelX, float contentWidth, float mouseX, float mouseY)
    {
        fill(255);
        textAlign(TextAlign::topLeft);
        textSize(13.0f);
        text("Brush size: " + std::to_string(static_cast<int>(ctx.brushSize)), panelX, 345.0f);

        constexpr float minBrush = 1.0f;
        constexpr float maxBrush = 80.0f;

        const float_rect sliderRect {panelX, 368.0f, contentWidth, 14.0f};
        noStroke();
        fill(60, 60, 64);
        rect(sliderRect.left, sliderRect.top, sliderRect.width, sliderRect.height, BorderRadius::circular(7.0f));

        const float t = std::clamp((ctx.brushSize - minBrush) / (maxBrush - minBrush), 0.0f, 1.0f);
        fill(90, 140, 220);
        circle(sliderRect.left + t * sliderRect.width, sliderRect.top + sliderRect.height * 0.5f, 10.0f);

        if (sliderRect.contains(mouseX, mouseY) and isMousePressed(MouseButton::left)) {
            ui.draggingBrushSlider = true;
        }
        if (ui.draggingBrushSlider) {
            if (isMouseDown(MouseButton::left)) {
                const float dt = std::clamp((mouseX - sliderRect.left) / sliderRect.width, 0.0f, 1.0f);
                ctx.brushSize = minBrush + dt * (maxBrush - minBrush);
            } else {
                ui.draggingBrushSlider = false;
            }
        }
    }

    void drawLayersPanel(UiState& ui, UiContext& ctx, float panelX, float contentWidth, float mouseX, float mouseY)
    {
        fill(255);
        textAlign(TextAlign::topLeft);
        textSize(15.0f);
        text("Layers", panelX, 398.0f);

        std::vector<Layer>& layers = ctx.canvas.layers();
        const float rowHeight = 28.0f;
        float rowY = 422.0f;

        // Displayed topmost-first (last vector index = drawn on top).
        for (size_t i = layers.size(); i-- > 0;) {
            const bool isActive = (i == ctx.canvas.activeLayerIndex());
            const float_rect rowRect {panelX, rowY, contentWidth, rowHeight - 4.0f};
            const float_rect visibilityRect {rowRect.left + 2.0f, rowRect.top + 2.0f, 22.0f, rowRect.height - 4.0f};

            noStroke();
            fill(isActive ? rgba(70, 90, 130) : rgba(50, 50, 54));
            rect(rowRect.left, rowRect.top, rowRect.width, rowRect.height, BorderRadius::circular(4.0f));

            fill(layers[i].visible ? rgba(255, 255, 255) : rgba(120, 120, 120));
            textAlign(TextAlign::center);
            textSize(13.0f);
            text(layers[i].visible ? "o" : "-", visibilityRect.left + visibilityRect.width * 0.5f, visibilityRect.top + visibilityRect.height * 0.5f);

            fill(255);
            textAlign(TextAlign::centerLeft);
            textSize(13.0f);
            text(layers[i].name, rowRect.left + 30.0f, rowRect.top + rowRect.height * 0.5f, rowRect.width - 34.0f);

            if (visibilityRect.contains(mouseX, mouseY) and isMousePressed(MouseButton::left)) {
                layers[i].visible = not layers[i].visible;
            } else if (rowRect.contains(mouseX, mouseY) and isMousePressed(MouseButton::left)) {
                ctx.canvas.setActiveLayerIndex(i);
            }

            rowY += rowHeight;
        }

        const float buttonY = rowY + 6.0f;
        const float buttonGap = 6.0f;
        const float buttonWidth = (contentWidth - 3.0f * buttonGap) / 4.0f;
        static constexpr const char* kLabels[4] = {"+", "-", "^", "v"};

        for (int i = 0; i < 4; ++i) {
            const float_rect r {panelX + static_cast<float>(i) * (buttonWidth + buttonGap), buttonY, buttonWidth, 28.0f};
            drawButton(r, kLabels[i], false, mouseX, mouseY);

            if (r.contains(mouseX, mouseY) and isMousePressed(MouseButton::left)) {
                const size_t active = ctx.canvas.activeLayerIndex();
                ctx.history.push(ctx.canvas.captureState());
                switch (i) {
                    case 0: ctx.canvas.addLayer("Layer " + std::to_string(ctx.canvas.layers().size() + 1)); break;
                    case 1: ctx.canvas.removeLayer(active); break;
                    case 2: ctx.canvas.moveLayerUp(active); break;
                    case 3: ctx.canvas.moveLayerDown(active); break;
                    default: break;
                }
            }
        }

        const float opacityY = buttonY + 40.0f;
        fill(255);
        textAlign(TextAlign::topLeft);
        textSize(13.0f);
        text("Opacity (active layer)", panelX, opacityY);

        const float_rect opacityRect {panelX, opacityY + 20.0f, contentWidth, 14.0f};
        noStroke();
        fill(60, 60, 64);
        rect(opacityRect.left, opacityRect.top, opacityRect.width, opacityRect.height, BorderRadius::circular(7.0f));

        Layer& activeLayer = ctx.canvas.activeLayer();
        fill(90, 140, 220);
        circle(opacityRect.left + activeLayer.opacity * opacityRect.width, opacityRect.top + opacityRect.height * 0.5f, 10.0f);

        if (opacityRect.contains(mouseX, mouseY) and isMousePressed(MouseButton::left)) {
            ui.draggingOpacity = true;
        }
        if (ui.draggingOpacity) {
            if (isMouseDown(MouseButton::left)) {
                activeLayer.opacity = std::clamp((mouseX - opacityRect.left) / opacityRect.width, 0.0f, 1.0f);
            } else {
                ui.draggingOpacity = false;
            }
        }
    }
} // namespace

namespace paint
{
    void initUi(UiState& ui, Font font)
    {
        ui.font = font;
        rebuildHueStrip(ui);
    }

    bool isOverUiChrome(float screenX, int windowWidth)
    {
        return screenX <= TOOLBAR_WIDTH or screenX >= static_cast<float>(windowWidth) - PANEL_WIDTH;
    }

    bool updateUi(UiState& ui, UiContext& ctx, int windowWidth, int windowHeight)
    {
        if (ctx.primaryColor != ui.lastSyncedPrimary) {
            rgbToHsv(ctx.primaryColor, ui.hue, ui.saturation, ui.value);
        }

        const bool toolChanged = drawToolbar(ctx, windowHeight);

        const float panelX = static_cast<float>(windowWidth) - PANEL_WIDTH;
        const float padding = 15.0f;
        const float contentWidth = PANEL_WIDTH - padding * 2.0f;
        const float mouseX = static_cast<float>(getMouseX());
        const float mouseY = static_cast<float>(getMouseY());

        noStroke();
        fill(32, 32, 36);
        rect(panelX, 0.0f, PANEL_WIDTH, static_cast<float>(windowHeight));

        drawColorPicker(ui, ctx, panelX + padding, contentWidth, mouseX, mouseY);
        drawBrushSlider(ui, ctx, panelX + padding, contentWidth, mouseX, mouseY);
        drawLayersPanel(ui, ctx, panelX + padding, contentWidth, mouseX, mouseY);

        ui.lastSyncedPrimary = ctx.primaryColor;
        return toolChanged;
    }
} // namespace paint
