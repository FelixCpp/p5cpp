#pragma once

namespace p5cpp
{
    struct BlendMode
    {
        enum class Factor {
            zero,
            one,
            srcColor,
            oneMinusSrcColor,
            dstColor,
            oneMinusDstColor,
            srcAlpha,
            oneMinusSrcAlpha,
            dstAlpha,
            oneMinusDstAlpha,
        };

        enum class Equation {
            add,
            subtract,
            reverseSubtract,
            min,
            max,
        };

        constexpr BlendMode(Factor srcColorFactor, Factor dstColorFactor, Equation colorEquation, Factor srcAlphaFactor, Factor dstAlphaFactor, Equation alphaEquation);

        inline constexpr bool operator==(const BlendMode& other) const = default;

        static const BlendMode none;
        static const BlendMode alpha;
        static const BlendMode additive;
        static const BlendMode multiply;
        static const BlendMode screen;
        static const BlendMode overlay;
        static const BlendMode darken;
        static const BlendMode lighten;
        static const BlendMode colorDodge;
        static const BlendMode colorBurn;

        Factor srcColorFactor;
        Factor dstColorFactor;
        Equation colorEquation;

        Factor srcAlphaFactor;
        Factor dstAlphaFactor;
        Equation alphaEquation;
    };
} // namespace p5cpp

namespace p5cpp
{
    constexpr BlendMode::BlendMode(Factor srcColorFactor, Factor dstColorFactor, Equation colorEquation, Factor srcAlphaFactor, Factor dstAlphaFactor, Equation alphaEquation)
        : srcColorFactor(srcColorFactor),
          dstColorFactor(dstColorFactor),
          colorEquation(colorEquation),
          srcAlphaFactor(srcAlphaFactor),
          dstAlphaFactor(dstAlphaFactor),
          alphaEquation(alphaEquation)
    {
    }

    inline constexpr BlendMode BlendMode::none = BlendMode(Factor::one, Factor::zero, Equation::add, Factor::one, Factor::zero, Equation::add);
    inline constexpr BlendMode BlendMode::alpha = BlendMode(Factor::srcAlpha, Factor::oneMinusSrcAlpha, Equation::add, Factor::one, Factor::zero, Equation::add);
    inline constexpr BlendMode BlendMode::additive = BlendMode(Factor::srcAlpha, Factor::one, Equation::add, Factor::one, Factor::zero, Equation::add);
    inline constexpr BlendMode BlendMode::multiply = BlendMode(Factor::dstColor, Factor::zero, Equation::add, Factor::one, Factor::zero, Equation::add);
    inline constexpr BlendMode BlendMode::screen = BlendMode(Factor::oneMinusDstColor, Factor::one, Equation::add, Factor::one, Factor::zero, Equation::add);
    inline constexpr BlendMode BlendMode::overlay = BlendMode(Factor::one, Factor::zero, Equation::add, Factor::one, Factor::zero, Equation::add);
    inline constexpr BlendMode BlendMode::darken = BlendMode(Factor::one, Factor::one, Equation::min, Factor::one, Factor::zero, Equation::add);
    inline constexpr BlendMode BlendMode::lighten = BlendMode(Factor::one, Factor::one, Equation::max, Factor::one, Factor::zero, Equation::add);
    inline constexpr BlendMode BlendMode::colorDodge = BlendMode(Factor::one, Factor::oneMinusSrcColor, Equation::add, Factor::one, Factor::zero, Equation::add);
    inline constexpr BlendMode BlendMode::colorBurn = BlendMode(Factor::oneMinusDstColor, Factor::one, Equation::add, Factor::one, Factor::zero, Equation::add);
} // namespace p5cpp
