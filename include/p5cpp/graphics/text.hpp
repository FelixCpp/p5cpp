#pragma once

namespace p5cpp
{
    enum class VerticalTextAlign {
        top,
        center,
        bottom,
        baseline,
    };

    enum class HorizontalTextAlign {
        left,
        center,
        right,
    };

    struct TextAlign
    {
        HorizontalTextAlign horizontal;
        VerticalTextAlign vertical;

        static const TextAlign topLeft;
        static const TextAlign topCenter;
        static const TextAlign topRight;

        static const TextAlign centerLeft;
        static const TextAlign center;
        static const TextAlign centerRight;

        static const TextAlign bottomLeft;
        static const TextAlign bottomCenter;
        static const TextAlign bottomRight;

        static const TextAlign baselineLeft;
        static const TextAlign baselineCenter;
        static const TextAlign baselineRight;
    };
} // namespace p5cpp

namespace p5cpp
{
    enum class TextWrap {
        none,
        word,
        character,
    };
}

namespace p5cpp
{
    inline constexpr TextAlign TextAlign::topLeft = TextAlign {.horizontal = HorizontalTextAlign::left, .vertical = VerticalTextAlign::top};
    inline constexpr TextAlign TextAlign::topCenter = TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::top};
    inline constexpr TextAlign TextAlign::topRight = TextAlign {.horizontal = HorizontalTextAlign::right, .vertical = VerticalTextAlign::top};
    inline constexpr TextAlign TextAlign::centerLeft = TextAlign {.horizontal = HorizontalTextAlign::left, .vertical = VerticalTextAlign::center};
    inline constexpr TextAlign TextAlign::center = TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::center};
    inline constexpr TextAlign TextAlign::centerRight = TextAlign {.horizontal = HorizontalTextAlign::right, .vertical = VerticalTextAlign::center};
    inline constexpr TextAlign TextAlign::bottomLeft = TextAlign {.horizontal = HorizontalTextAlign::left, .vertical = VerticalTextAlign::bottom};
    inline constexpr TextAlign TextAlign::bottomCenter = TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::bottom};
    inline constexpr TextAlign TextAlign::bottomRight = TextAlign {.horizontal = HorizontalTextAlign::right, .vertical = VerticalTextAlign::bottom};
    inline constexpr TextAlign TextAlign::baselineLeft = TextAlign {.horizontal = HorizontalTextAlign::left, .vertical = VerticalTextAlign::baseline};
    inline constexpr TextAlign TextAlign::baselineCenter = TextAlign {.horizontal = HorizontalTextAlign::center, .vertical = VerticalTextAlign::baseline};
    inline constexpr TextAlign TextAlign::baselineRight = TextAlign {.horizontal = HorizontalTextAlign::right, .vertical = VerticalTextAlign::baseline};
} // namespace p5cpp
