#pragma once

#include "p5.hpp"
#include "drawscope.hpp"

#include <array>
#include <vector>

namespace p5
{
    struct DrawCall
    {
        size_t indexOffset;
        size_t indexCount;

        BlendMode blendMode;

        std::shared_ptr<Shader> shader;
        std::vector<UniformVariable> shaderUniforms;

        std::array<uint32_t, 8> textureUnits;
        size_t textureUnitCount;
    };

    typedef std::vector<DrawCall> DrawCallList;

    void draw_calls_submit(DrawScope& scope, DrawCallList& drawCalls, std::shared_ptr<Shader> shader, BlendMode blendMode, uint32_t texture);
} // namespace p5
