#include "p5.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <string>

using namespace p5;

// ── Helpers ───────────────────────────────────────────────────────────────────

static constexpr float PI = std::numbers::pi_v<float>;

// ─────────────────────────────────────────────────────────────────────────────
//  ShowcaseSketch
//  Row 1 – Stroke Joins  : miter / bevel / round  (5-pointed star)
//  Row 2 – Stroke Caps   : butt  / square / round (open lines, 3 weights)
//  Row 3 – Shape Gallery : line / rect / circle / triangle / arc / bezier
//  Row 4 – Edge Cases    : near-U-turn · razor spikes · SW>>segs ·
//                          sharp V+miter-limit · open-path caps · concave polygon
//  Row 5 – Supershapes   : heart · rose · Lissajous · astroid · superellipse · spirograph
// ─────────────────────────────────────────────────────────────────────────────

struct ShowcaseSketch : p5::Sketch
{
    static constexpr int WIN_W = 1200;
    static constexpr int VIEWPORT_H = 840; // visible window height
    static constexpr int CONTENT_H = 1400; // total content height (5 × ROW_H)
    static constexpr float COL3_W = WIN_W / 3.f;
    static constexpr float COL6_W = WIN_W / 6.f;
    static constexpr float ROW_H = CONTENT_H / 5.f; // 280 px per row
    static constexpr float TITLE_H = 22.f;
    static constexpr float SCROLL_SPD = 40.f; // px per scroll tick

    float scrollY = 0.f;
    float time = 0.f; // accumulated seconds, used by all animations

    void setup() override
    {
        setWindowSize(WIN_W, VIEWPORT_H);
        setWindowTitle("Stroke Joins & Caps – Showcase  [↑↓ / scroll to navigate]");
        setWindowResizable(false);
    }

    void destroy() override {}

    // Returns a value oscillating smoothly in [0,1] — used by all animations.
    float osc(float speed, float phase = 0.f) const
    {
        return 0.5f + 0.5f * std::sin(time * speed + phase);
    }

    void draw() override
    {
        time += deltaTime;
        background(22, 22, 26);

        // ── Scrollable content ────────────────────────────────────────────
        pushMatrix();
        translate(0.f, -scrollY);

        // ── Row 1: Join types ────────────────────────────────────────────
        sectionTitle("Stroke Joins", 10.f, 4.f);
        {
            const StrokeJoin joins[] = {StrokeJoin::miter, StrokeJoin::bevel, StrokeJoin::round};
            const char* labels[] = {"Miter", "Bevel", "Round"};
            for (int i = 0; i < 3; ++i)
                drawJoinPanel(i * COL3_W, TITLE_H, COL3_W, ROW_H - TITLE_H, joins[i], labels[i]);
        }

        // ── Row 2: Cap styles ────────────────────────────────────────────
        const float row2Y = ROW_H;
        sectionTitle("Stroke Caps", 10.f, row2Y + 4.f);
        {
            const StrokeCap caps[] = {StrokeCap::butt, StrokeCap::square, StrokeCap::round};
            const char* labels[] = {"Butt", "Square", "Round"};
            for (int i = 0; i < 3; ++i)
                drawCapPanel(i * COL3_W, row2Y + TITLE_H, COL3_W, ROW_H - TITLE_H, caps[i], labels[i]);
        }

        // ── Row 3: Primitive shapes ──────────────────────────────────────
        const float row3Y = ROW_H * 2.f;
        sectionTitle("Shape Gallery  (Round join / Round cap / strokeWeight 6)", 10.f, row3Y + 4.f);
        {
            const char* labels[] = {"line()", "rect()", "circle()", "triangle()", "arc()  pie", "bezier()"};
            for (int i = 0; i < 6; ++i)
                drawShapePanel(i * COL6_W, row3Y + TITLE_H, COL6_W, ROW_H - TITLE_H, i, labels[i]);
        }

        // ── Row 4: Edge cases ────────────────────────────────────────────
        const float row4Y = ROW_H * 3.f;
        sectionTitle("Edge Cases  (near-U-turn · razor spikes · SW>>segs · sharp V+miter · open caps · concave)", 10.f, row4Y + 4.f);
        {
            const char* labels[] = {"U-turn joins", "Razor spikes", "SW >> segs", "Sharp V (miter)", "Open path caps", "Concave polygon"};
            for (int i = 0; i < 6; ++i)
                drawEdgePanel(i * COL6_W, row4Y + TITLE_H, COL6_W, ROW_H - TITLE_H, i, labels[i]);
        }

        // ── Row 5: Supershapes ───────────────────────────────────────────
        const float row5Y = ROW_H * 4.f;
        sectionTitle("Supershapes  (heart · rose · Lissajous · astroid · superellipse · spirograph)", 10.f, row5Y + 4.f);
        {
            const char* labels[] = {"Herz", "Rosenblüte", "Lissajous", "Astroid", "Superellipse", "Spirograph"};
            for (int i = 0; i < 6; ++i)
                drawSuperPanel(i * COL6_W, row5Y + TITLE_H, COL6_W, ROW_H - TITLE_H, i, labels[i]);
        }

        popMatrix();

        // ── Scrollbar (fixed, outside the scrolling transform) ────────────
        drawScrollbar();
    }

    void drawScrollbar()
    {
        constexpr float BAR_W = 6.f;
        constexpr float MARGIN = 2.f;
        const float maxScroll = static_cast<float>(CONTENT_H - VIEWPORT_H);
        const float barH = static_cast<float>(VIEWPORT_H) - MARGIN * 2.f;
        const float thumbH = barH * static_cast<float>(VIEWPORT_H) / static_cast<float>(CONTENT_H);
        const float thumbY = MARGIN + (scrollY / maxScroll) * (barH - thumbH);
        const float barX = static_cast<float>(WIN_W) - BAR_W - MARGIN;

        noStroke();
        fill(45, 45, 55);
        rect(barX, MARGIN, BAR_W, barH);

        fill(110, 110, 140);
        rect(barX, thumbY, BAR_W, thumbH);
    }

    void event(const WindowEvent& e) override
    {
        const float maxScroll = static_cast<float>(CONTENT_H - VIEWPORT_H);

        if (e.type == EventType::mouseScroll) {
            scrollY -= e.mouseScroll.dy * SCROLL_SPD;
            scrollY = std::clamp(scrollY, 0.f, maxScroll);
        }

        if (e.type == EventType::keyPress) {
            switch (e.keyEvent.key) {
                case Key::up: scrollY -= SCROLL_SPD; break;
                case Key::down: scrollY += SCROLL_SPD; break;
                case Key::pageUp: scrollY -= ROW_H; break;
                case Key::pageDown: scrollY += ROW_H; break;
                case Key::home: scrollY = 0.f; break;
                case Key::end: scrollY = maxScroll; break;
                default: break;
            }
            scrollY = std::clamp(scrollY, 0.f, maxScroll);
        }
    }

    // ─── Shared helpers ─────────────────────────────────────────────────────

    void sectionTitle(const char* label, float x, float y)
    {
        noStroke();
        fill(190, 190, 210);
        textAlign(HorizontalTextAlign::left, VerticalTextAlign::top);
        textSize(12.f);
        text(label, x, y);
    }

    void panelBackground(float x, float y, float w, float h, const char* label)
    {
        noStroke();
        fill(30, 30, 36);
        rect(x + 2.f, y + 2.f, w - 4.f, h - 4.f);

        noFill();
        stroke(52, 52, 66);
        strokeWeight(1.f);
        rect(x + 2.f, y + 2.f, w - 4.f, h - 4.f);

        noStroke();
        fill(150, 150, 170);
        textAlign(HorizontalTextAlign::center, VerticalTextAlign::top);
        textSize(12.f);
        text(label, x + w / 2.f, y + 7.f);
    }

    // ─── Row 1 – Join demos ──────────────────────────────────────────────────

    void drawJoinPanel(float x, float y, float w, float h, StrokeJoin join, const char* label)
    {
        panelBackground(x, y, w, h, label);

        const float cx = x + w / 2.f;
        const float cy = y + h / 2.f + 8.f;
        const float outerR = std::min(w, h) * 0.35f;
        // innerR oscillates: razor-thin (2 %) → normal star (44 %)
        const float phase = x / static_cast<float>(WIN_W) * PI * 2.f;
        const float innerR = outerR * (0.02f + 0.42f * osc(0.55f, phase));
        // strokeWeight pulses 6 → 16
        const float sw = 6.f + 10.f * osc(0.45f, phase + 1.f);
        // slow rotation
        const float rot = time * 0.3f + phase * 0.2f;

        strokeJoin(join);
        strokeCap(StrokeCap::butt);
        strokeWeight(sw);
        stroke(255, 195, 60);
        noFill();

        // 5-pointed star, rotated over time
        beginShape();
        for (int i = 0; i < 10; ++i) {
            const float angle = rot + 2.f * PI * i / 10.f - PI / 2.f;
            const float r = (i % 2 == 0) ? outerR : innerR;
            vertex(cx + std::cos(angle) * r, cy + std::sin(angle) * r);
        }
        endShape(ShapeType::lineLoop, true);
    }

    // ─── Row 2 – Cap demos ───────────────────────────────────────────────────

    void drawCapPanel(float x, float y, float w, float h, StrokeCap cap, const char* label)
    {
        panelBackground(x, y, w, h, label);

        const float phase = x / static_cast<float>(WIN_W) * PI * 2.f;
        const float baseMargin = w * 0.17f;
        // Lines grow and shrink — margin oscillates inward/outward
        const float extra = w * 0.10f * osc(0.6f, phase);
        const float x0 = x + baseMargin + extra;
        const float x1 = x + w - baseMargin - extra;
        const float usableH = h - 40.f;
        const float step = usableH / 4.f; // 3 lines in 4 slots → even spacing

        // strokeWeight pulses differently for each line
        for (int i = 0; i < 3; ++i) {
            const float ly = y + 35.f + step * static_cast<float>(i + 1);
            const float sw = 4.f + 18.f * osc(0.65f, phase + static_cast<float>(i) * 1.2f);

            stroke(70, 150, 245);
            strokeWeight(sw);
            strokeCap(cap);
            noFill();
            line(x0, ly, x1, ly);

            // Thin tick marks at the exact endpoint positions
            stroke(90, 90, 110);
            strokeWeight(1.f);
            strokeCap(StrokeCap::butt);
            const float tick = sw * 0.65f + 2.f;
            line(x0, ly - tick, x0, ly + tick);
            line(x1, ly - tick, x1, ly + tick);
        }
    }

    // ─── Row 3 – Shape gallery ───────────────────────────────────────────────

    void drawShapePanel(float x, float y, float w, float h, int shapeIndex, const char* label)
    {
        panelBackground(x, y, w, h, label);

        const float phase = static_cast<float>(shapeIndex) * 0.9f;
        // strokeWeight pulses 2 → 12
        const float sw = 2.f + 10.f * osc(0.5f, phase);
        // arc sweep grows 20° → 340°
        const float sweep = (20.f + 320.f * osc(0.35f, phase + 0.5f)) * PI / 180.f;

        strokeJoin(StrokeJoin::round);
        strokeCap(StrokeCap::round);
        strokeWeight(sw);
        stroke(75, 205, 125);
        fill(38, 70, 52, 190);

        const float cx = x + w / 2.f;
        const float cy = y + h / 2.f + 10.f;
        const float r = std::min(w, h) * 0.30f;

        switch (shapeIndex) {
            case 0: // line() — endpoint oscillates vertically
                noFill();
                line(cx - r, cy - r * 0.5f * osc(0.7f, phase), cx + r, cy + r * 0.5f * osc(0.7f, phase + PI));
                break;

            case 1: // rect() — size pulses
            {
                const float rw = r * (1.2f + 0.6f * osc(0.4f, phase));
                const float rh = r * (0.9f + 0.4f * osc(0.4f, phase + 1.f));
                rect(cx - rw / 2.f, cy - rh / 2.f, rw, rh);
                break;
            }

            case 2: // circle() — radius pulses
                circle(cx, cy, r * 2.f * (0.6f + 0.5f * osc(0.5f, phase)));
                break;

            case 3: // triangle() — apex oscillates
                triangle(cx, cy - r * (0.8f + 0.4f * osc(0.55f, phase)), cx - r, cy + r * 0.7f, cx + r, cy + r * 0.7f);
                break;

            case 4: // arc() — sweep angle grows / shrinks
                arc(cx, cy, r * 2.f, r * 2.f, -PI / 2.f, sweep, ArcMode::pie);
                break;

            case 5: // bezier() — control points breathe
            {
                const float lift = r * (0.7f + 0.5f * osc(0.45f, phase));
                noFill();
                bezier(cx - r, cy + r * 0.5f, cx - r * 0.3f, cy - lift, cx + r * 0.3f, cy - lift, cx + r, cy + r * 0.5f);
                break;
            }
        }
    }
    // ─── Row 4 – Edge case dispatcher ───────────────────────────────────────

    void drawEdgePanel(float x, float y, float w, float h, int index, const char* label)
    {
        panelBackground(x, y, w, h, label);
        const float cx = x + w / 2.f;
        const float cy = y + h / 2.f + 8.f;
        const float r = std::min(w, h) * 0.28f;
        switch (index) {
            case 0: edge_UTurnJoins(x, y, w, h); break;
            case 1: edge_RazorSpikes(cx, cy, r); break;
            case 2: edge_ThickShortSegs(cx, cy); break;
            case 3: edge_SharpVMiter(x, y, w, h, cx); break;
            case 4: edge_OpenPathCaps(x, y, w, h, cx); break;
            case 5: edge_ConcavePolygon(cx, cy, r); break;
        }
    }

    // ── Edge 0: same hairpin (~174° turn) drawn with all three join types ──

    void edge_UTurnJoins(float x, float y, float w, float h)
    {
        const StrokeJoin joins[] = {StrokeJoin::miter, StrokeJoin::bevel, StrokeJoin::round};
        const char* jnames[] = {"miter", "bevel", "round"};
        const float rowH = (h - 30.f) / 3.f;
        const float cx = x + w / 2.f;
        const float arm = w * 0.30f;
        // gap oscillates 0.5 → 22 px — the near-zero case is where the old innerHit bug fired
        const float gap = 0.5f + 21.5f * osc(0.4f);

        for (int i = 0; i < 3; ++i) {
            const float ry = y + 30.f + rowH * (i + 0.5f);
            // strokeWeight pulses independently per row
            const float sw = 5.f + 8.f * osc(0.5f, static_cast<float>(i) * 1.2f);

            strokeJoin(joins[i]);
            strokeCap(StrokeCap::butt);
            strokeWeight(sw);
            stroke(255, 195, 60);
            noFill();

            beginShape();
            vertex(cx - arm, ry - gap / 2.f);
            vertex(cx + arm * 0.5f, ry - gap / 2.f);
            vertex(cx - arm, ry + gap / 2.f);
            endShape(ShapeType::lineStrip, false);

            noStroke();
            fill(90, 90, 115);
            textSize(9.f);
            textAlign(HorizontalTextAlign::left, VerticalTextAlign::center);
            text(jnames[i], x + 5.f, ry);
        }
    }

    // ── Edge 1: 5-pointed star with near-zero-degree spike tips ────────────

    void edge_RazorSpikes(float cx, float cy, float r)
    {
        // innerR oscillates: 1 % (hair-thin) → 30 % (normal star)
        const float innerR = r * (0.01f + 0.29f * osc(0.5f));
        // slow rotation to stress all angle orientations
        const float rot = time * 0.4f;

        strokeJoin(StrokeJoin::round);
        strokeCap(StrokeCap::round);
        strokeWeight(4.f);
        stroke(255, 90, 90);
        noFill();

        beginShape();
        for (int i = 0; i < 10; ++i) {
            const float angle = rot + 2.f * PI * i / 10.f - PI / 2.f;
            const float rad = (i % 2 == 0) ? r : innerR;
            vertex(cx + std::cos(angle) * rad, cy + std::sin(angle) * rad);
        }
        endShape(ShapeType::lineLoop, true);
    }

    // ── Edge 2: strokeWeight >> segment length ─────────────────────────────

    void edge_ThickShortSegs(float cx, float cy)
    {
        // amplitude oscillates 2 → 18 px — changes how steep each segment is
        const float amp = 2.f + 16.f * osc(0.6f);
        // strokeWeight also pulses so sw/segLen ratio swings widely
        const float sw = 8.f + 14.f * osc(0.45f, 0.8f);
        const int N = 9;
        const float stepX = 10.f;

        strokeJoin(StrokeJoin::round);
        strokeCap(StrokeCap::round);
        strokeWeight(sw);
        stroke(100, 200, 255);
        noFill();

        beginShape();
        for (int i = 0; i < N; ++i) {
            const float vx = cx - stepX * (N - 1) / 2.f + i * stepX;
            const float vy = cy + (i % 2 == 0 ? -amp : amp);
            vertex(vx, vy);
        }
        endShape(ShapeType::lineStrip, false);
    }

    // ── Edge 3: V-shapes at three included angles — exercises miter limit ──

    void edge_SharpVMiter(float x, float y, float w, float h, float cx)
    {
        // Each row sweeps its included angle independently → dynamic miter-limit transitions
        // Row 0: sweeps 2° → 18° (crosses the bevel fallback threshold around 11°)
        // Row 1: sweeps 10° → 60° (stays in miter zone)
        // Row 2: sweeps 30° → 90° (always miter)
        const float minHalf[] = {radians(1.f), radians(5.f), radians(15.f)};
        const float maxHalf[] = {radians(9.f), radians(30.f), radians(45.f)};
        const char* angLabels[] = {"~5° (threshold)", "10–60° range", "30–90° range"};
        const float rowH = (h - 30.f) / 3.f;
        const float arm = w * 0.30f;

        for (int i = 0; i < 3; ++i) {
            const float phase = static_cast<float>(i) * 1.1f;
            const float halfAngle = minHalf[i] + (maxHalf[i] - minHalf[i]) * osc(0.4f, phase);
            const float ry = y + 30.f + rowH * (i + 0.5f);
            const float tipY = ry + arm * 0.15f;
            const float endDX = arm * std::sin(halfAngle);
            const float endY = tipY - arm * std::cos(halfAngle);
            const float sw = 3.f + 7.f * osc(0.55f, phase + 2.f);

            strokeJoin(StrokeJoin::miter);
            strokeCap(StrokeCap::butt);
            strokeWeight(sw);
            stroke(180, 130, 255);
            noFill();

            beginShape();
            vertex(cx - endDX, endY);
            vertex(cx, tipY);
            vertex(cx + endDX, endY);
            endShape(ShapeType::lineStrip, false);

            noStroke();
            fill(90, 90, 115);
            textSize(9.f);
            textAlign(HorizontalTextAlign::left, VerticalTextAlign::center);
            text(angLabels[i], x + 5.f, ry);
        }
    }

    // ── Edge 4: open zigzag — all three cap styles stacked ─────────────────

    void edge_OpenPathCaps(float x, float y, float w, float h, float cx)
    {
        const StrokeCap caps[] = {StrokeCap::butt, StrokeCap::square, StrokeCap::round};
        const char* cnames[] = {"butt", "square", "round"};
        const float rowH = (h - 30.f) / 3.f;
        const float step = w * 0.14f;

        for (int i = 0; i < 3; ++i) {
            const float ry = y + 30.f + rowH * (i + 0.5f);
            // strokeWeight pulses with independent phase per row
            const float sw = 3.f + 15.f * osc(0.5f, static_cast<float>(i) * 1.1f);
            // amplitude also shifts so the zigzag shape changes
            const float amp = 5.f + 8.f * osc(0.65f, static_cast<float>(i) * 0.7f + 1.5f);

            strokeJoin(StrokeJoin::round);
            strokeCap(caps[i]);
            strokeWeight(sw);
            stroke(80, 200, 160);
            noFill();

            beginShape();
            vertex(cx - step * 1.5f, ry + amp);
            vertex(cx - step * 0.5f, ry - amp);
            vertex(cx + step * 0.5f, ry + amp);
            vertex(cx + step * 1.5f, ry - amp);
            endShape(ShapeType::lineStrip, false);

            // Tick marks at the exact start/end endpoints
            stroke(80, 80, 100);
            strokeWeight(1.f);
            strokeCap(StrokeCap::butt);
            const float tick = sw * 0.7f + 2.f;
            line(cx - step * 1.5f, ry + amp - tick, cx - step * 1.5f, ry + amp + tick);
            line(cx + step * 1.5f, ry - amp - tick, cx + step * 1.5f, ry - amp + tick);

            noStroke();
            fill(90, 90, 115);
            textSize(9.f);
            textAlign(HorizontalTextAlign::left, VerticalTextAlign::center);
            text(cnames[i], x + 5.f, ry);
        }
    }

    // ── Edge 5: 12-vertex plus/cross — exercises concave (reflex) corners ──

    void edge_ConcavePolygon(float cx, float cy, float r)
    {
        // arm half-width pulses: thin arms ↔ fat arms
        const float a = r * (0.18f + 0.22f * osc(0.4f));
        const float b = r;
        // slow continuous rotation
        const float rot = time * 0.25f;

        strokeJoin(StrokeJoin::round);
        strokeCap(StrokeCap::round);
        strokeWeight(5.f);
        stroke(255, 165, 80);
        fill(80, 55, 25, 190);

        pushMatrix();
        translate(cx, cy);
        rotate(rot);

        // 12-vertex plus sign (4 convex 90° + 8 concave 270° corners), centred at origin
        beginShape();
        vertex(-a, -b);
        vertex(a, -b);
        vertex(a, -a);
        vertex(b, -a);
        vertex(b, a);
        vertex(a, a);
        vertex(a, b);
        vertex(-a, b);
        vertex(-a, a);
        vertex(-b, a);
        vertex(-b, -a);
        vertex(-a, -a);
        endShape(ShapeType::lineLoop, true);

        popMatrix();
    }

    // ─── Row 5 – Supershapes ─────────────────────────────────────────────────

    void drawSuperPanel(float x, float y, float w, float h, int idx, const char* label)
    {
        panelBackground(x, y, w, h, label);
        const float cx = x + w / 2.f;
        const float cy = y + h / 2.f + 8.f;
        const float r = std::min(w, h) * 0.38f;
        switch (idx) {
            case 0: super_Heart(cx, cy, r); break;
            case 1: super_Rose(cx, cy, r); break;
            case 2: super_Lissajous(cx, cy, r); break;
            case 3: super_Astroid(cx, cy, r); break;
            case 4: super_Superellipse(cx, cy, r); break;
            case 5: super_Spirograph(cx, cy, r); break;
        }
    }

    // Parametric heart:  x = 16 sin³ t,  y = 13 cos t − 5 cos 2t − 2 cos 3t − cos 4t
    // y-centre of mass ≈ −2.5 (math coords)  →  screen_y = cy − (y + 2.5) · scale
    void super_Heart(float cx, float cy, float r)
    {
        const float scale = r / 16.f;
        // Heartbeat: product of two sines creates a realistic double-thump pattern
        const float beat = osc(1.1f) * osc(2.2f);
        const float sc = 1.f + 0.10f * beat;
        const float sw = 2.5f + 3.5f * beat;

        strokeJoin(StrokeJoin::miter);
        strokeCap(StrokeCap::butt);
        strokeWeight(sw);
        stroke(255, 75, 115);
        fill(170, 25, 55, 110);

        const int N = 120;
        beginShape();
        for (int i = 0; i < N; ++i) {
            const float t = 2.f * PI * i / N;
            const float hx = 16.f * std::sin(t) * std::sin(t) * std::sin(t);
            const float hy = 13.f * std::cos(t) - 5.f * std::cos(2.f * t) - 2.f * std::cos(3.f * t) - std::cos(4.f * t);
            vertex(cx + hx * scale * sc, cy - (hy + 2.5f) * scale * sc);
        }
        endShape(ShapeType::lineLoop, true);
    }

    // Polar rose  r = cos(k · θ)  — k morphs 1.5 → 5.0 → 1.5 (petals grow / split)
    void super_Rose(float cx, float cy, float r)
    {
        const float k = 1.5f + 3.5f * osc(0.12f);
        const float rot = time * 0.18f;
        const float sw = 1.5f + 3.f * osc(0.4f, 1.f);

        strokeJoin(StrokeJoin::round);
        strokeCap(StrokeCap::round);
        strokeWeight(sw);
        stroke(190, 100, 255);
        fill(55, 15, 95, 85);

        // 0..4π captures all petals for non-integer k
        const int N = 800;
        beginShape();
        for (int i = 0; i < N; ++i) {
            const float theta = 4.f * PI * i / N;
            const float rad = r * std::cos(k * theta);
            vertex(cx + rad * std::cos(theta + rot), cy + rad * std::sin(theta + rot));
        }
        endShape(ShapeType::lineStrip, false);
    }

    // Lissajous  x = r sin(3t + φ),  y = r sin(2t)  — phase φ drifts → figure morphs
    void super_Lissajous(float cx, float cy, float r)
    {
        const float phi = time * 0.55f;
        const float sw = 1.5f + 2.5f * osc(0.35f, 0.7f);

        strokeJoin(StrokeJoin::round);
        strokeCap(StrokeCap::round);
        strokeWeight(sw);
        stroke(70, 215, 230);
        noFill();

        const int N = 600;
        beginShape();
        for (int i = 0; i <= N; ++i) {
            const float t = 2.f * PI * i / N;
            vertex(cx + r * std::sin(3.f * t + phi), cy + r * std::sin(2.f * t));
        }
        endShape(ShapeType::lineStrip, false);
    }

    // Astroid  x = a cos³ t,  y = b sin³ t
    // a and b breathe in opposite phase → morphs between tall and wide
    void super_Astroid(float cx, float cy, float r)
    {
        const float a = r * (0.65f + 0.40f * osc(0.28f));
        const float b = r * (0.65f + 0.40f * osc(0.28f, PI));
        const float sw = 1.5f + 4.f * osc(0.45f, 1.3f);

        strokeJoin(StrokeJoin::round);
        strokeCap(StrokeCap::round);
        strokeWeight(sw);
        stroke(255, 205, 50);
        fill(75, 55, 5, 85);

        const int N = 240;
        beginShape();
        for (int i = 0; i < N; ++i) {
            const float t = 2.f * PI * i / N;
            const float ct = std::cos(t), st = std::sin(t);
            vertex(cx + a * ct * ct * ct, cy + b * st * st * st);
        }
        endShape(ShapeType::lineLoop, true);
    }

    // Superellipse  |x/r|^n + |y/r|^n = 1
    // n morphs: 0.5 (pointy star) → 1 (diamond) → 2 (circle) → 5 (squircle)
    void super_Superellipse(float cx, float cy, float r)
    {
        const float n = 0.5f + 4.5f * osc(0.18f, 0.5f);
        const float exp_ = 2.f / n;
        const float sw = 1.5f + 3.f * osc(0.42f, 2.f);

        strokeJoin(StrokeJoin::round);
        strokeCap(StrokeCap::round);
        strokeWeight(sw);
        stroke(85, 230, 140);
        fill(15, 65, 30, 85);

        const int N = 320;
        beginShape();
        for (int i = 0; i < N; ++i) {
            const float t = 2.f * PI * i / N;
            const float ct = std::cos(t), st = std::sin(t);
            vertex(cx + std::copysign(std::pow(std::abs(ct), exp_), ct) * r, cy + std::copysign(std::pow(std::abs(st), exp_), st) * r);
        }
        endShape(ShapeType::lineLoop, true);
    }

    // Hypotrochoid (spirograph)  R:r = 5:3, tracing arm d oscillates
    // Curve closes after t = 0..6π
    void super_Spirograph(float cx, float cy, float r)
    {
        const float R_ = r * 5.f / 8.f;
        const float ri_ = r * 3.f / 8.f;
        const float k_ = (R_ - ri_) / ri_; // ≈ 0.667
        const float d = ri_ * (0.4f + 2.0f * osc(0.22f));
        const float sw = 1.5f + 2.5f * osc(0.4f, 1.8f);

        strokeJoin(StrokeJoin::round);
        strokeCap(StrokeCap::round);
        strokeWeight(sw);
        stroke(255, 145, 75);
        noFill();

        const int N = 900;
        beginShape();
        for (int i = 0; i <= N; ++i) {
            const float t = 6.f * PI * i / N;
            vertex(cx + (R_ - ri_) * std::cos(t) + d * std::cos(k_ * t), cy + (R_ - ri_) * std::sin(t) - d * std::sin(k_ * t));
        }
        endShape(ShapeType::lineStrip, false);
    }
};

namespace p5
{
    std::unique_ptr<p5::Sketch> createSketch()
    {
        return std::make_unique<ShowcaseSketch>();
    }
} // namespace p5
