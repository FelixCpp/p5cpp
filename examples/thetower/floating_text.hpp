#pragma once

#include <string>
#include <vector>

#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

struct FloatingText
{
    std::string text;
    p5::float2 position;
    p5::color_t color;
    float size;

    float riseSpeed;
    float lifetime;
    float age;

    p5::animation::ValueTweenTransition<float> pop;
};

FloatingText floating_text_create(std::string text, const p5::float2& position, p5::color_t color, float size, float riseSpeed, float lifetime);
void floating_texts_update(std::vector<FloatingText>& texts, float deltaTime);
void floating_texts_show(const std::vector<FloatingText>& texts, const p5::Font& font);
