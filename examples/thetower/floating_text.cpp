#include "floating_text.hpp"

#include <algorithm>

using namespace p5;
using namespace p5::animation;

namespace
{
    // the last part of the lifetime is spent fading out
    constexpr float fadeStart = 0.6f;
    constexpr float riseDrag = 2.5f;
}

FloatingText floating_text_create(std::string text, const p5::float2& position, const p5::color_t color, const float size, const float riseSpeed, const float lifetime)
{
    return FloatingText {
        .text = std::move(text),
        .position = position,
        .color = color,
        .size = size,
        .riseSpeed = riseSpeed,
        .lifetime = lifetime,
        .age = 0.0f,
        .pop = valueTween(0.0f, 1.0f, 0.3f, curves::easeOutBack),
    };
}

void floating_texts_update(std::vector<FloatingText>& texts, const float deltaTime)
{
    for (FloatingText& text : texts) {
        text.age += deltaTime;
        text.pop.advance(deltaTime);
        text.position.y -= text.riseSpeed * deltaTime;
        text.riseSpeed = damp(text.riseSpeed, 0.0f, riseDrag, deltaTime);
    }

    std::erase_if(texts, [](const FloatingText& text) {
        return text.age >= text.lifetime;
    });
}

void floating_texts_show(const std::vector<FloatingText>& texts, const p5::Font& font)
{
    with([&texts, &font] {
        textFont(font);
        textAlign(TextAlignment::center);
        noStroke();

        for (const FloatingText& text : texts) {
            const float t = text.age / text.lifetime;
            const float alpha = t < fadeStart ? 1.0f : 1.0f - curves::easeInQuad((t - fadeStart) / (1.0f - fadeStart));

            textSize(text.size * std::max(text.pop.value(), 0.01f));
            fill(withOpacity(text.color, alpha));
            p5::text(text.text, text.position.x, text.position.y);
        }
    });
}
