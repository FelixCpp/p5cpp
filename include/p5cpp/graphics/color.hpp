#pragma once

#include <cstdint>

namespace p5cpp
{
    typedef uint32_t color_t;

    constexpr color_t rgba(int grey, int alpha = 255);
    constexpr color_t rgba(int red, int green, int blue, int alpha = 255);

    constexpr color_t lighten(color_t color, float factor);
    constexpr color_t darken(color_t color, float factor);

    constexpr color_t lerp(color_t color1, color_t color2, float t);
    constexpr color_t withAlpha(color_t color, int alpha);

    constexpr int red(color_t color);
    constexpr int green(color_t color);
    constexpr int blue(color_t color);
    constexpr int alpha(color_t color);
    constexpr int brightness(color_t color);
} // namespace p5cpp

namespace p5cpp
{
    inline constexpr color_t rgba(int grey, int alpha)
    {
        return rgba(grey, grey, grey, alpha);
    }

    inline constexpr color_t rgba(int red, int green, int blue, int alpha)
    {
        return (static_cast<color_t>(red) << 24) | (static_cast<color_t>(green) << 16) | (static_cast<color_t>(blue) << 8) | static_cast<color_t>(alpha);
    }

    inline constexpr color_t lighten(color_t color, float factor)
    {
        int r = red(color);
        int g = green(color);
        int b = blue(color);
        int a = alpha(color);

        r = static_cast<int>(r + (255 - r) * factor);
        g = static_cast<int>(g + (255 - g) * factor);
        b = static_cast<int>(b + (255 - b) * factor);

        return rgba(r, g, b, a);
    }

    inline constexpr color_t darken(color_t color, float factor)
    {
        int r = red(color);
        int g = green(color);
        int b = blue(color);
        int a = alpha(color);

        r = static_cast<int>(r * (1.0f - factor));
        g = static_cast<int>(g * (1.0f - factor));
        b = static_cast<int>(b * (1.0f - factor));

        return rgba(r, g, b, a);
    }

    inline constexpr color_t lerp(color_t color1, color_t color2, float t)
    {
        const int r = static_cast<int>(red(color1) + (red(color2) - red(color1)) * t);
        const int g = static_cast<int>(green(color1) + (green(color2) - green(color1)) * t);
        const int b = static_cast<int>(blue(color1) + (blue(color2) - blue(color1)) * t);
        const int a = static_cast<int>(alpha(color1) + (alpha(color2) - alpha(color1)) * t);

        return rgba(r, g, b, a);
    }

    inline constexpr color_t withAlpha(color_t color, int alpha) { return (color & 0xFFFFFF00) | (static_cast<color_t>(alpha) & 0xFF); }
    inline constexpr int red(color_t color) { return (color >> 24) & 0xFF; }
    inline constexpr int green(color_t color) { return (color >> 16) & 0xFF; }
    inline constexpr int blue(color_t color) { return (color >> 8) & 0xFF; }
    inline constexpr int alpha(color_t color) { return color & 0xFF; }

    inline constexpr int brightness(color_t color)
    {
        const int r = red(color);
        const int g = green(color);
        const int b = blue(color);

        return static_cast<int>(0.299f * r + 0.587f * g + 0.114f * b);
    }
} // namespace p5cpp
