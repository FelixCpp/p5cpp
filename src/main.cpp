#include "p5.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <random>
#include <string>
#include <vector>

using namespace p5;

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string fmtf(float v, int dp)
{
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.*f", dp, static_cast<double>(v));
    return buf;
}

// ── Neural Network  2 → H → 1 ─────────────────────────────────────────────────
// Hidden activation: tanh   |   Output activation: sigmoid
// Training:         full-batch gradient descent with binary cross-entropy loss

struct Net
{
    int H = 0;
    std::vector<float> w1, b1; // input→hidden  (H×2), (H)
    std::vector<float> w2, b2; // hidden→output (H),   (1)
    std::vector<float> hAct;   // hidden activations, reused across forward()

    void init(int hidden, std::mt19937& rng)
    {
        H = hidden;
        std::normal_distribution<float> d(0.f, 0.8f);
        w1.resize(H * 2);
        b1.assign(H, 0.f);
        w2.resize(H);
        b2.assign(1, 0.f);
        hAct.resize(H);
        for (auto& w : w1) w = d(rng);
        for (auto& w : w2) w = d(rng);
    }

    // Stateless forward pass – used for heatmap and accuracy queries.
    float predict(float x0, float x1) const
    {
        float o = b2[0];
        for (int j = 0; j < H; ++j) {
            float a = std::tanh(w1[j * 2] * x0 + w1[j * 2 + 1] * x1 + b1[j]);
            o += w2[j] * a;
        }
        return 1.f / (1.f + std::exp(-o));
    }

    // Forward pass that caches hidden activations for backprop.
    float forward(float x0, float x1)
    {
        float o = b2[0];
        for (int j = 0; j < H; ++j) {
            hAct[j] = std::tanh(w1[j * 2] * x0 + w1[j * 2 + 1] * x1 + b1[j]);
            o += w2[j] * hAct[j];
        }
        return 1.f / (1.f + std::exp(-o));
    }

    // One full-batch GD step; returns mean cross-entropy loss.
    float trainStep(const std::vector<std::pair<float2, float>>& data, float lr)
    {
        if (data.empty()) return 0.f;

        std::vector<float> dw1(H * 2, 0.f), db1(H, 0.f), dw2(H, 0.f);
        float db2 = 0.f, totalLoss = 0.f;

        for (auto& [pt, lbl] : data) {
            float p = forward(pt.x, pt.y);
            totalLoss += -(lbl * std::log(p + 1e-7f) + (1.f - lbl) * std::log(1.f - p + 1e-7f));
            float dO = p - lbl; // dL / d(pre-activation output)
            db2 += dO;
            for (int j = 0; j < H; ++j) {
                dw2[j] += dO * hAct[j];
                float dH = dO * w2[j] * (1.f - hAct[j] * hAct[j]); // tanh′
                dw1[j * 2] += dH * pt.x;
                dw1[j * 2 + 1] += dH * pt.y;
                db1[j] += dH;
            }
        }

        float s = lr / static_cast<float>(data.size());
        b2[0] -= s * db2;
        for (int j = 0; j < H; ++j) {
            w2[j] -= s * dw2[j];
            w1[j * 2] -= s * dw1[j * 2];
            w1[j * 2 + 1] -= s * dw1[j * 2 + 1];
            b1[j] -= s * db1[j];
        }
        return totalLoss / static_cast<float>(data.size());
    }

    void reset(std::mt19937& rng)
    {
        std::normal_distribution<float> d(0.f, 0.8f);
        std::fill(b1.begin(), b1.end(), 0.f);
        b2[0] = 0.f;
        for (auto& w : w1) w = d(rng);
        for (auto& w : w2) w = d(rng);
    }
};

// ── Sketch ────────────────────────────────────────────────────────────────────

struct NNSketch : p5::Sketch
{
    // ── Layout ───────────────────────────────────────────────────────────
    static constexpr int FIELD_W = 700;
    static constexpr int FIELD_H = 700;
    static constexpr int PANEL_W = 280;
    static constexpr int HM_RES = 70; // heatmap canvas resolution per axis
    static constexpr int HIDDEN = 4;
    static constexpr float LR = 0.05f;
    static constexpr int STEPS_PER_FRAME = 20;
    static constexpr float HM_INTERVAL = 0.05f; // seconds between heatmap rebuilds

    // ── State ────────────────────────────────────────────────────────────
    Net nn;
    std::mt19937 rng {42};

    struct Point
    {
        float2 norm, screen;
        float label;
    };
    std::vector<Point> points;
    std::vector<std::pair<float2, float>> trainSet; // mirrors points, avoids alloc per step

    bool training = true;
    float loss = 0.f;
    int epoch = 0;

    std::shared_ptr<Framebuffer> heatmapCanvas;
    float hmTimer = 0.f;

    bool lmbDown = false, rmbDown = false;
    float2 lastPt = {-9999.f, -9999.f};

    // ── Lifecycle ────────────────────────────────────────────────────────

    void setup() override
    {
        setWindowSize(FIELD_W + PANEL_W, FIELD_H);
        setWindowTitle("Neural Network  –  2D Classifier");
        setWindowResizable(false);
        // frameRate(60);

        nn.init(HIDDEN, rng);
        heatmapCanvas = createCanvas(HM_RES, HM_RES);
        rebuildHeatmap();
    }

    void destroy() override {}

    // ── Draw ─────────────────────────────────────────────────────────────

    void draw() override
    {
        if (training && !trainSet.empty()) {
            for (int i = 0; i < STEPS_PER_FRAME; ++i)
                loss = nn.trainStep(trainSet, LR);
            epoch += STEPS_PER_FRAME;
        }

        hmTimer += deltaTime;
        if (hmTimer >= HM_INTERVAL) {
            hmTimer = 0.f;
            rebuildHeatmap();
        }

        background(20, 20, 28);
        drawField();
        drawPanel();

        // stroke(255, 0, 0);
        // fill(0, 255, 0);
        // strokeWeight(15.f);
        // rect(100.0f, 100.0f, 300.0f, 300.0f);
    }

    // ── Heatmap ──────────────────────────────────────────────────────────

    void rebuildHeatmap()
    {
        pushCanvas(heatmapCanvas);
        noStroke();
        for (int row = 0; row < HM_RES; ++row) {
            for (int col = 0; col < HM_RES; ++col) {
                float nx = (col + .5f) / HM_RES * 2.f - 1.f;
                float ny = (row + .5f) / HM_RES * 2.f - 1.f;
                float v = nn.predict(nx, ny);
                int r = static_cast<int>(v * 190.f);
                int b = static_cast<int>((1.f - v) * 190.f);
                fill(20 + r, 20, 20 + b);

                rect(static_cast<float>(col), static_cast<float>(row), 1.0f, 1.0f);
            }
        }
        popCanvas();
    }

    // ── Field (left 700 × 700) ────────────────────────────────────────────

    void drawField()
    {
        noTint();
        noStroke();
        image(heatmapCanvas->getTextureId(), 0.f, 0.f, FIELD_W, FIELD_H);

        // Subtle grid
        stroke(255, 255, 255, 18);
        strokeWeight(1.f);
        for (int i = 1; i < 4; ++i) {
            line(FIELD_W * i / 4.f, 0.f, FIELD_W * i / 4.f, FIELD_H);
            line(0.f, FIELD_H * i / 4.f, FIELD_W, FIELD_H * i / 4.f);
        }
        // Center axes
        stroke(255, 255, 255, 40);
        line(FIELD_W * .5f, 0.f, FIELD_W * .5f, FIELD_H);
        line(0.f, FIELD_H * .5f, FIELD_W, FIELD_H * .5f);
        noStroke();

        // Data points
        for (auto& p : points) {
            if (p.label > .5f) {
                fill(255, 80, 80);
                stroke(255, 200, 200);
            } else {
                fill(80, 120, 255);
                stroke(180, 200, 255);
            }
            strokeWeight(1.5f);
            circle(p.screen.x, p.screen.y, 12.f);
        }
        noStroke();

        // FPS overlay (top-left)
        textSize(13.f);
        textAlign(HorizontalTextAlign::left, VerticalTextAlign::top);
        fill(160, 255, 160, 200);
        text("FPS " + std::to_string(static_cast<int>(fps + .5f)), 8.f, 6.f);
    }

    // ── Panel (right 280 px) ──────────────────────────────────────────────

    void drawPanel()
    {
        float px = static_cast<float>(FIELD_W);

        noStroke();
        fill(14, 14, 22);
        rect(px, 0.f, static_cast<float>(PANEL_W), static_cast<float>(FIELD_H));

        stroke(50, 50, 72);
        strokeWeight(1.f);
        line(px, 0.f, px, static_cast<float>(FIELD_H));
        noStroke();

        drawNNDiagram(px);
        drawStats(px);
    }

    // ── NN diagram ────────────────────────────────────────────────────────

    // Returns the y-coordinate for node `idx` in a layer of `count` nodes,
    // evenly distributed between [top, top+span].
    static float nodeY(int count, int idx, float top, float span)
    {
        if (count == 1) return top + span * .5f;
        return top + span * static_cast<float>(idx) / static_cast<float>(count - 1);
    }

    void drawWeightLine(float x1, float y1, float x2, float y2, float w)
    {
        float wAbs = std::min(std::abs(w), 3.f);
        int a = static_cast<int>(wAbs / 3.f * 160.f) + 20;
        if (w > 0.f) stroke(255, 130, 80, a);
        else stroke(80, 150, 255, a);
        strokeWeight(wAbs * .7f + .3f);
        line(x1, y1, x2, y2);
    }

    void drawNNDiagram(float px)
    {
        constexpr int IN = 2;
        constexpr int HID = HIDDEN;

        float cx0 = px + 50.f;  // input column x
        float cx1 = px + 140.f; // hidden column x
        float cx2 = px + 230.f; // output column x

        float yTop = 30.f, yBot = 460.f, span = yBot - yTop;

        // Input nodes occupy a narrower vertical range for visual balance
        float inTop = yTop + 90.f, inSpan = span - 180.f;

        // ── Connections: input → hidden ───────────────────────────────
        for (int j = 0; j < HID; ++j) {
            float yh = nodeY(HID, j, yTop, span);
            for (int i = 0; i < IN; ++i)
                drawWeightLine(cx0, nodeY(IN, i, inTop, inSpan), cx1, yh, nn.w1[j * 2 + i]);
        }

        // ── Connections: hidden → output ──────────────────────────────
        float yOut = nodeY(1, 0, yTop, span);
        for (int j = 0; j < HID; ++j)
            drawWeightLine(cx1, nodeY(HID, j, yTop, span), cx2, yOut, nn.w2[j]);
        noStroke();

        // ── Input nodes ───────────────────────────────────────────────
        const char* inLabels[] = {"x", "y"};
        for (int i = 0; i < IN; ++i) {
            float y = nodeY(IN, i, inTop, inSpan);
            fill(70, 150, 240);
            stroke(160, 200, 255);
            strokeWeight(1.5f);
            circle(cx0, y, 18.f);
            fill(255);
            noStroke();
            textSize(10.f);
            textAlign(HorizontalTextAlign::center, VerticalTextAlign::center);
            text(inLabels[i], cx0, y);
        }

        // ── Hidden nodes (brightness reflects bias) ───────────────────
        for (int j = 0; j < HID; ++j) {
            float y = nodeY(HID, j, yTop, span);
            float t = std::tanh(nn.b1[j]) * .5f + .5f;
            int br = static_cast<int>(t * 120.f) + 30;
            fill(br, br, br + 30);
            stroke(130, 130, 180);
            strokeWeight(1.f);
            circle(cx1, y, 12.f);
        }
        noStroke();

        // ── Output node (color reflects prediction at mouse position) ──
        {
            float pred = .5f;
            if (mouseX >= 0 && mouseX < FIELD_W && mouseY >= 0 && mouseY < FIELD_H) {
                float mx = (static_cast<float>(mouseX) / FIELD_W) * 2.f - 1.f;
                float my = (static_cast<float>(mouseY) / FIELD_H) * 2.f - 1.f;
                pred = nn.predict(mx, my);
            }
            int r = static_cast<int>(pred * 200.f);
            int b = static_cast<int>((1.f - pred) * 200.f);
            fill(40 + r, 40, 40 + b);
            stroke(200, 200, 220);
            strokeWeight(1.5f);
            circle(cx2, yOut, 20.f);
            fill(255);
            noStroke();
            textSize(9.f);
            textAlign(HorizontalTextAlign::center, VerticalTextAlign::center);
            text("out", cx2, yOut);
        }

        // ── Layer labels ──────────────────────────────────────────────
        textSize(10.f);
        textAlign(HorizontalTextAlign::center, VerticalTextAlign::top);
        fill(90, 90, 130);
        text("Input", cx0, yBot + 8.f);
        text("Hidden", cx1, yBot + 8.f);
        text("Output", cx2, yBot + 8.f);
    }

    // ── Stats / controls panel ────────────────────────────────────────────

    void drawStats(float px)
    {
        float y = 488.f;
        float xl = px + 12.f;
        float xr = px + PANEL_W - 12.f;

        // Loss label + value
        textSize(12.f);
        textAlign(HorizontalTextAlign::left, VerticalTextAlign::top);
        fill(180, 180, 200);
        text("Loss", xl, y);
        textAlign(HorizontalTextAlign::right, VerticalTextAlign::top);
        fill(210, 210, 220);
        text(fmtf(loss, 4), xr, y);

        // Loss bar
        float barX = xl + 36.f, barY = y + 3.f, barW = 138.f, barH = 10.f;
        noStroke();
        fill(35, 35, 50);
        rect(barX, barY, barW, barH);
        fill(255, 160, 60);
        rect(barX, barY, barW * (1.f - std::min(loss, 2.f) / 2.f), barH);

        y += 20.f;
        textSize(12.f);
        textAlign(HorizontalTextAlign::left, VerticalTextAlign::top);

        fill(150, 200, 150);
        text("Epoch  " + std::to_string(epoch), xl, y);
        y += 18.f;

        int nRed = 0, nBlue = 0;
        for (auto& p : points) {
            if (p.label > .5f) ++nRed;
            else ++nBlue;
        }
        fill(255, 100, 100);
        text("● Red   " + std::to_string(nRed), xl, y);
        y += 16.f;
        fill(100, 140, 255);
        text("● Blue  " + std::to_string(nBlue), xl, y);
        y += 20.f;

        fill(training ? color(100, 255, 120) : color(255, 200, 60));
        text(training ? "● Training" : "  Paused", xl, y);
        y += 20.f;

        // Divider
        stroke(50, 50, 70);
        strokeWeight(1.f);
        line(xl, y, xr, y);
        noStroke();
        y += 10.f;

        // Controls
        textSize(11.f);
        fill(95, 95, 130);
        textAlign(HorizontalTextAlign::left, VerticalTextAlign::top);

        const char* ctrls[] = {
            "[LMB]   add red point",
            "[RMB]   add blue point",
            "[drag]  paint points",
            "[C]     clear all points",
            "[R]     reset network",
            "[Space] pause / resume",
        };
        for (auto* s : ctrls) {
            text(s, xl, y);
            y += 14.f;
        }
    }

    // ── Events ────────────────────────────────────────────────────────────

    void event(const WindowEvent& e) override
    {
        switch (e.type) {
            case EventType::mousePress: {
                int mx = e.mouseButton.x, my = e.mouseButton.y;
                if (e.mouseButton.button == MouseButton::left) lmbDown = true;
                if (e.mouseButton.button == MouseButton::right) rmbDown = true;
                if (mx >= 0 && mx < FIELD_W && my >= 0 && my < FIELD_H)
                    addPoint(mx, my, lmbDown ? 1.f : 0.f);
                break;
            }
            case EventType::mouseRelease:
                if (e.mouseButton.button == MouseButton::left) lmbDown = false;
                if (e.mouseButton.button == MouseButton::right) rmbDown = false;
                break;
            case EventType::mouseMove: {
                if (!lmbDown && !rmbDown) break;
                int mx = e.mouseMove.x, my = e.mouseMove.y;
                if (mx < 0 || mx >= FIELD_W || my < 0 || my >= FIELD_H) break;
                float2 cur = {static_cast<float>(mx), static_cast<float>(my)};
                float2 d = cur - lastPt;
                if (d.x * d.x + d.y * d.y < 18.f * 18.f) break; // throttle
                addPoint(mx, my, lmbDown ? 1.f : 0.f);
                break;
            }
            case EventType::keyPress:
                switch (e.keyEvent.key) {
                    case Key::space: training = !training; break;
                    case Key::r:
                        nn.reset(rng);
                        loss = 0.f;
                        epoch = 0;
                        rebuildHeatmap();
                        break;
                    case Key::c:
                        points.clear();
                        trainSet.clear();
                        loss = 0.f;
                        epoch = 0;
                        break;
                    default: break;
                }
                break;
            default: break;
        }
    }

    void addPoint(int sx, int sy, float label)
    {
        float nx = (static_cast<float>(sx) / FIELD_W) * 2.f - 1.f;
        float ny = (static_cast<float>(sy) / FIELD_H) * 2.f - 1.f;
        points.push_back(Point {{nx, ny}, {static_cast<float>(sx), static_cast<float>(sy)}, label});
        trainSet.push_back({{nx, ny}, label});
        lastPt = {static_cast<float>(sx), static_cast<float>(sy)};
    }
};

namespace p5
{
    std::unique_ptr<p5::Sketch> createSketch()
    {
        return std::make_unique<NNSketch>();
    }
} // namespace p5
