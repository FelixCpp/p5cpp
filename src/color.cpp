#include <p5.hpp>

#include <algorithm>

namespace p5
{
    color_t rgba(int grey, int alpha)
    {
        return rgba(grey, grey, grey, alpha);
    }

    color_t rgba(int red, int green, int blue, int alpha)
    {
        const int r = std::clamp(red, 0, 255);
        const int g = std::clamp(green, 0, 255);
        const int b = std::clamp(blue, 0, 255);
        const int a = std::clamp(alpha, 0, 255);
        return (r << 24) | (g << 16) | (b << 8) | a;
    }

    color_t lighten(color_t color, float amount)
    {
        return rgba(
            std::min(255, static_cast<int>(red(color) + amount * 255)),
            std::min(255, static_cast<int>(green(color) + amount * 255)),
            std::min(255, static_cast<int>(blue(color) + amount * 255)),
            alpha(color)
        );
    }

    color_t darken(color_t color, float amount)
    {
        return rgba(
            std::max(0, static_cast<int>(red(color) - amount * 255)),
            std::max(0, static_cast<int>(green(color) - amount * 255)),
            std::max(0, static_cast<int>(blue(color) - amount * 255)),
            alpha(color)
        );
    }

    color_t lerp(color_t a, color_t b, float t)
    {
        return rgba(
            static_cast<int>(red(a) + t * (red(b) - red(a))),
            static_cast<int>(green(a) + t * (green(b) - green(a))),
            static_cast<int>(blue(a) + t * (blue(b) - blue(a))),
            static_cast<int>(alpha(a) + t * (alpha(b) - alpha(a)))
        );
    }

    color_t withAlpha(color_t color, int alpha)
    {
        const int a = std::clamp(alpha, 0, 255);
        return (color & 0xFFFFFF00) | a;
    }

    int red(color_t color)
    {
        return (color & 0xFF000000) >> 24;
    }

    int green(color_t color)
    {
        return (color & 0x00FF0000) >> 16;
    }

    int blue(color_t color)
    {
        return (color & 0x0000FF00) >> 8;
    }

    int alpha(color_t color)
    {
        return (color & 0x000000FF) >> 0;
    }

    int brightness(color_t color)
    {
        const int r = red(color);
        const int g = green(color);
        const int b = blue(color);

        return static_cast<int>(0.299f * r + 0.587f * g + 0.114f * b);
    }
} // namespace p5
