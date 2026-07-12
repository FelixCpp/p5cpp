#include <p5cpp/graphics/color.hpp>

#include <cmath>

namespace p5cpp
{
    color_t hsv(float h, float s, float v, int a)
    {
        h = std::fmod(h, 360.f);
        if (h < 0.f) h += 360.f;
        float r, g, b;
        const int i = int(h / 60.f) % 6;
        const float f = h / 60.f - int(h / 60.f);
        const float p = v * (1.f - s);
        const float q = v * (1.f - f * s);
        const float u = v * (1.f - (1.f - f) * s);
        switch (i) {
            case 0:
                r = v;
                g = u;
                b = p;
                break;
            case 1:
                r = q;
                g = v;
                b = p;
                break;
            case 2:
                r = p;
                g = v;
                b = u;
                break;
            case 3:
                r = p;
                g = q;
                b = v;
                break;
            case 4:
                r = u;
                g = p;
                b = v;
                break;
            default:
                r = v;
                g = p;
                b = q;
                break;
        }

        return rgba(static_cast<int>((r) * 255), static_cast<int>((g) * 255), static_cast<int>((b) * 255), a);
    }
} // namespace p5cpp
