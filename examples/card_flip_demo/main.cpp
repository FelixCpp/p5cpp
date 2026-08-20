#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>
#include <vector>

using namespace p5;

// Demonstrates the classic "cosine scale-X" trick for a 2D card flip: no real 3D rotation, just a
// horizontal scale animated through zero. This is the building block a bigger card game (a
// deckbuilding roguelike) will lean on for hand animations, so it's kept as its own small example.
namespace
{
    enum class Element
    {
        Fire,
        Water,
        Earth,
        Air,
    };
    constexpr int kElementCount = 4;

    color_t elementColor(Element element)
    {
        switch (element) {
            case Element::Fire: return rgba(224, 86, 60);
            case Element::Water: return rgba(60, 140, 224);
            case Element::Earth: return rgba(90, 168, 90);
            case Element::Air: return rgba(205, 205, 214);
        }
        return rgba(255, 255, 255);
    }

    std::string_view elementName(Element element)
    {
        switch (element) {
            case Element::Fire: return "Fire";
            case Element::Water: return "Water";
            case Element::Earth: return "Earth";
            case Element::Air: return "Air";
        }
        return "?";
    }

    // Every icon is built from the plain primitives (triangle/circle/square), so the example has no
    // texture/asset dependency and stays copy-paste portable.
    void drawElementIcon(Element element, float cx, float cy, float size, color_t color)
    {
        noStroke();
        fill(color);
        switch (element) {
            case Element::Fire:
                triangle(cx, cy - size * 0.55f, cx - size * 0.5f, cy + size * 0.4f, cx + size * 0.5f, cy + size * 0.4f);
                break;
            case Element::Water:
                circle(cx, cy, size * 0.45f);
                break;
            case Element::Earth:
                square(cx - size * 0.35f, cy - size * 0.35f, size * 0.7f);
                break;
            case Element::Air:
                push();
                translate(cx, cy);
                rotate(std::numbers::pi_v<float> * 0.25f);
                square(-size * 0.32f, -size * 0.32f, size * 0.64f);
                pop();
                break;
        }
    }

    struct Card
    {
        Element element = Element::Fire;
        int rank = 1;
        float x = 0.0f, y = 0.0f; // top-left, in window space

        bool faceUp = false; // the state the flip animation is heading towards / resting at
        float flipT = 1.0f;  // 0 = flip just started, 1 = at rest (not animating)
        float hoverT = 0.0f; // 0..1, eased hover lift
    };
} // namespace

struct CardFlipDemoSketch : public Sketch
{
    void setup() override
    {
        setWindowSize(820, 700);
        setWindowTitle("p5cpp - Card Flip Demo");
        buildDeck();
    }

    void draw() override
    {
        if (isKeyPressed(Key::R)) {
            enterFullscreen();
        }

        if (isKeyPressed(Key::F)) {
            leaveFullscreen();
        }

        if (isKeyPressed(Key::T)) {
            toggleFullscreen();
        }

        updateCards();
        handleInput();
        render();
    }

private:
    static constexpr int kCols = 4;
    static constexpr int kRows = 3;
    static constexpr float kCardWidth = 140.0f;
    static constexpr float kCardHeight = 190.0f;
    static constexpr float kGap = 24.0f;
    static constexpr float kFlipDuration = 0.35f; // seconds for a full face-to-face flip

    void buildDeck()
    {
        const uint2 windowSize = getWindowSize();
        const float gridWidth = kCols * kCardWidth + (kCols - 1) * kGap;
        const float gridHeight = kRows * kCardHeight + (kRows - 1) * kGap;
        const float startX = (static_cast<float>(windowSize.x) - gridWidth) * 0.5f;
        const float startY = (static_cast<float>(windowSize.y) - gridHeight) * 0.5f + 30.0f;

        m_cards.reserve(kCols * kRows);
        for (int row = 0; row < kRows; ++row) {
            for (int col = 0; col < kCols; ++col) {
                m_cards.push_back(Card {
                    .element = static_cast<Element>(std::min(static_cast<int>(random(static_cast<float>(kElementCount))), kElementCount - 1)),
                    .rank = 1 + std::min(static_cast<int>(random(9.0f)), 8),
                    .x = startX + static_cast<float>(col) * (kCardWidth + kGap),
                    .y = startY + static_cast<float>(row) * (kCardHeight + kGap),
                });
            }
        }
    }

    void updateCards()
    {
        const float dt = static_cast<float>(getDeltaTime());
        const float mouseX = static_cast<float>(getMouseX());
        const float mouseY = static_cast<float>(getMouseY());

        for (Card& card : m_cards) {
            const bool hovered = mouseX >= card.x and mouseX <= card.x + kCardWidth and mouseY >= card.y and mouseY <= card.y + kCardHeight;
            const float hoverTarget = (hovered and card.flipT >= 1.0f) ? 1.0f : 0.0f;
            card.hoverT += (hoverTarget - card.hoverT) * std::min(1.0f, dt * 12.0f);

            if (card.flipT < 1.0f) card.flipT = std::min(card.flipT + dt / kFlipDuration, 1.0f);
        }
    }

    void handleInput()
    {
        if (not isMouseButtonPressed(MouseButton::Left)) return;

        const float mouseX = static_cast<float>(getMouseX());
        const float mouseY = static_cast<float>(getMouseY());
        for (Card& card : m_cards) {
            if (card.flipT < 1.0f) continue; // ignore clicks on a card that's still mid-flip
            if (mouseX < card.x or mouseX > card.x + kCardWidth or mouseY < card.y or mouseY > card.y + kCardHeight) continue;

            // Flip target state is decided here, at the start of the animation; drawCard() below
            // derives which face is actually visible each frame purely from flipT.
            card.faceUp = not card.faceUp;
            card.flipT = 0.0f;
            break;
        }
    }

    void render()
    {
        background(rgba(18, 18, 24));
        drawHeader();
        for (const Card& card : m_cards) drawCard(card);
    }

    void drawHeader()
    {
        const float centerX = static_cast<float>(getWindowSize().x) * 0.5f;

        noStroke();
        fill(rgba(230, 230, 235));
        textSize(22);
        textAlign(TextAlignment::topCenter);
        text("Card Flip Demo", centerX, 20.0f);

        fill(rgba(150, 150, 160));
        textSize(13);
        text("Click a card to flip it", centerX, 50.0f);
    }

    void drawCard(const Card& card)
    {
        const float centerX = card.x + kCardWidth * 0.5f;
        const float centerY = card.y + kCardHeight * 0.5f;

        // The trick: animate an angle from 0 to pi and use scaleX = |cos(angle)|. That goes
        // 1 -> 0 -> 1, i.e. "flat -> edge-on -> flat", exactly like a card turning over. Using the
        // absolute value (instead of the raw, signed cosine) means scaleX is never negative, so the
        // face drawn inside is never mirrored - only which face gets drawn changes, at the exact
        // moment the card is edge-on (angle == pi/2, where cos crosses zero).
        const float eased = easeOutBack(card.flipT); // small overshoot at the end = a little "settle" bounce
        const float angle = eased * std::numbers::pi_v<float>;
        const float raw = std::cos(angle);
        const float scaleX = std::abs(raw);

        // card.faceUp already holds the *target* face (it's toggled the instant a flip starts), so
        // the first half of the animation still needs to show the face we're flipping away from.
        const bool showFaceUp = (raw >= 0.0f) ? not card.faceUp : card.faceUp;

        const float flipLift = std::sin(std::min(card.flipT, 1.0f) * std::numbers::pi_v<float>) * 16.0f;
        const float lift = card.hoverT * 8.0f + flipLift;

        noStroke();
        fill(rgba(0, 0, 0, static_cast<int32_t>(90.0f * (1.0f - std::min(lift / 26.0f, 1.0f)))));
        ellipse(centerX, card.y + kCardHeight + 10.0f, kCardWidth * 0.4f * scaleX, 9.0f);

        push();
        translate(centerX, centerY - lift);
        scale(scaleX, 1.0f);
        if (showFaceUp) drawFace(card);
        else drawBack();
        pop();
    }

    void drawFace(const Card& card)
    {
        const color_t accent = elementColor(card.element);

        fill(rgba(250, 248, 240));
        stroke(accent);
        strokeWeight(2.5f);
        rect(-kCardWidth * 0.5f, -kCardHeight * 0.5f, kCardWidth, kCardHeight, BorderRadius::all(12.0f));

        drawElementIcon(card.element, 0.0f, -14.0f, 60.0f, accent);

        noStroke();
        fill(rgba(60, 60, 65));
        textAlign(TextAlignment::center);
        textSize(13);
        text(elementName(card.element), 0.0f, 46.0f);

        const std::string rankLabel = std::to_string(card.rank);
        fill(rgba(40, 40, 45));
        textSize(20);
        textAlign(TextAlignment::topLeft);
        text(rankLabel, -kCardWidth * 0.5f + 10.0f, -kCardHeight * 0.5f + 8.0f);

        // Same label again, rotated 180 degrees around the card's center, lands mirrored in the
        // opposite corner - the usual "readable from both ends" layout of a real playing card.
        push();
        rotate(std::numbers::pi_v<float>);
        text(rankLabel, -kCardWidth * 0.5f + 10.0f, -kCardHeight * 0.5f + 8.0f);
        pop();
    }

    void drawBack()
    {
        fill(rgba(30, 34, 54));
        stroke(rgba(255, 255, 255, 50));
        strokeWeight(2.5f);
        rect(-kCardWidth * 0.5f, -kCardHeight * 0.5f, kCardWidth, kCardHeight, BorderRadius::all(12.0f));

        noFill();
        stroke(rgba(255, 255, 255, 70));
        strokeWeight(1.5f);
        rect(-kCardWidth * 0.5f + 10.0f, -kCardHeight * 0.5f + 10.0f, kCardWidth - 20.0f, kCardHeight - 20.0f, BorderRadius::all(8.0f));

        noStroke();
        fill(rgba(255, 255, 255, 60));
        textAlign(TextAlignment::center);
        textSize(16);
        text("p5", 0.0f, 0.0f);
    }

    std::vector<Card> m_cards;
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<CardFlipDemoSketch>();
}
