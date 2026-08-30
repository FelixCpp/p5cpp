#include <p5cpp_animation/p5cpp_animation.hpp>

#include <cmath>

namespace p5::animation
{
    // clang-format off
    float easeLinear(float t) { return t; }
    float easeInQuad(float t) { return t * t; }
    float easeOutQuad(float t) { return t * (2.0f - t); }
    float easeInOutQuad(float t) { return (t < 0.5f) ? (2.0f * t * t) : (-1.0f + (4.0f - 2.0f * t) * t); }
    float easeInCubic(float t) { return t * t * t; }
    float easeOutCubic(float t) { const float u = t - 1.0f; return u * u * u + 1.0f; }
    float easeInOutCubic(float t) { return (t < 0.5f) ? (4.0f * t * t * t) : ((t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f); }
    float easeInQuart(float t) { return t * t * t * t; }
    float easeOutQuart(float t) { const float u = t - 1.0f; return 1.0f - u * u * u * u; }
    float easeInOutQuart(float t) { if (t < 0.5f) { return 8.0f * t * t * t * t; } const float u = t - 1.0f; return 1.0f - 8.0f * u * u * u * u; }
    float easeInQuint(float t) { return t * t * t * t * t; }
    float easeOutQuint(float t) { const float u = t - 1.0f; return 1.0f + u * u * u * u * u; }
    float easeInOutQuint(float t) { if (t < 0.5f) { return 16.0f * t * t * t * t * t; } const float u = t - 1.0f; return 1.0f + 16.0f * u * u * u * u * u; }
    float easeInSine(float t) { return 1.0f - std::cos((t * PI) / 2.0f); }
    float easeOutSine(float t) { return std::sin((t * PI) / 2.0f); }
    float easeInOutSine(float t) { return -(std::cos(PI * t) - 1.0f) / 2.0f; }
    float easeInExpo(float t) { return (t == 0.0f) ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f)); }
    float easeOutExpo(float t) { return (t == 1.0f) ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t); }
    float easeInOutExpo(float t) { return (t == 0.0f) ? 0.0f : (t == 1.0f) ? 1.0f : (t < 0.5f) ? std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f; }
    float easeInCirc(float t) { return 1.0f - std::sqrt(1.0f - t * t); }
    float easeOutCirc(float t) { const float u = t - 1.0f; return std::sqrt(1.0f - u * u); }
    float easeInOutCirc(float t) { if (t < 0.5f) { return (1.0f - std::sqrt(1.0f - 4.0f * t * t)) / 2.0f; } const float u = t - 1.0f; return (std::sqrt(1.0f - u * (2.0f * u)) + 1.0f) / 2.0f; }
    float easeInBack(float t) { const float s = 1.70158f; return t * t * ((s + 1.0f) * t - s); }
    float easeOutBack(float t) { const float s = 1.70158f; const float u = t - 1.0f; return u * u * ((s + 1.0f) * u + s) + 1.0f; }
    float easeInOutBack(float t) { const float s = 1.70158f * 1.525f; if (t < 0.5f) { return t * t * ((s + 1.0f) * 2.0f * t - s); } const float u = t - 1.0f; return u * u * ((s + 1.0f) * 2.0f * u + s) + 1.0f; }
    float easeInElastic(float t) { return (t == 0.0f) ? 0.0f : (t == 1.0f) ? 1.0f : -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * ((2.0f * PI) / 3.0f)); }
    float easeOutElastic(float t) { return (t == 0.0f) ? 0.0f : (t == 1.0f) ? 1.0f : std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * ((2.0f * PI) / 3.0f)) + 1.0f; }
    float easeInOutElastic(float t) { return (t == 0.0f) ? 0.0f : (t == 1.0f) ? 1.0f : (t < 0.5f) ? -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * ((2.0f * PI) / 4.5f))) / 2.0f : (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * ((2.0f * PI) / 4.5f))) / 2.0f + 1.0f; }
    float easeInBounce(float t) { return 1.0f - easeOutBounce(1.0f - t); }
    float easeOutBounce(float t) { if (t < 1.0f / 2.75f) { return 7.5625f * t * t; } else if (t < 2.0f / 2.75f) { t -= 1.5f / 2.75f; return 7.5625f * t * t + 0.75f; } else if (t < 2.5f / 2.75f) { t -= 2.25f / 2.75f; return 7.5625f * t * t + 0.9375f; } else { t -= 2.625f / 2.75f; return 7.5625f * t * t + 0.984375f; } }
    float easeInOutBounce(float t) { return (t < 0.5f) ? (1.0f - easeOutBounce(1.0f - 2.0f * t)) / 2.0f : (1.0f + easeOutBounce(2.0f * t - 1.0f)) / 2.0f; }
    // clang-format on
} // namespace p5::animation

namespace p5::animation
{
    float tween(float a, float b, float t, Curve curve)
    {
        return a + (b - a) * curve(t);
    }

    float2 tween(float2 a, float2 b, float t, Curve curve)
    {
        const float dx = tween(a.x, b.x, t, curve);
        const float cy = tween(a.y, b.y, t, curve);
        return float2 {.x = dx, .y = cy};
    }

    float3 tween(float3 a, float3 b, float t, Curve curve)
    {
        const float dx = tween(a.x, b.x, t, curve);
        const float cy = tween(a.y, b.y, t, curve);
        const float dz = tween(a.z, b.z, t, curve);
        return float3 {.x = dx, .y = cy, .z = dz};
    }

    float4 tween(float4 a, float4 b, float t, Curve curve)
    {
        const float dx = tween(a.x, b.x, t, curve);
        const float cy = tween(a.y, b.y, t, curve);
        const float dz = tween(a.z, b.z, t, curve);
        const float dw = tween(a.w, b.w, t, curve);
        return float4 {.x = dx, .y = cy, .z = dz, .w = dw};
    }
} // namespace p5::animation
