#pragma once

#include "p5.hpp"

#include <array>
#include <cstdint>
#include <memory>

namespace p5
{
    struct DrawCall
    {
        size_t indexOffset;
        size_t indexCount;

        BlendMode blendMode;

        std::shared_ptr<Shader> shader;

        std::array<uint32_t, 8> textureUnits;
        size_t textureUnitCount;
    };
} // namespace p5
