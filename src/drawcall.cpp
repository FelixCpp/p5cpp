#include "drawcall.hpp"

#include <cassert>

namespace p5
{
    static DrawCall& get_or_create_draw_call(DrawCallList& drawCalls, std::shared_ptr<Shader> shader, BlendMode blendMode, uint32_t texture, uint32_t baseIndex)
    {
        DrawCall* previousDrawCall = drawCalls.empty() ? nullptr : &drawCalls.back();

        const bool hasShaderChange = previousDrawCall != nullptr and previousDrawCall->shader != shader;
        const bool hasBlendModeChange = previousDrawCall != nullptr and previousDrawCall->blendMode != blendMode;

        if (previousDrawCall == nullptr or hasShaderChange or hasBlendModeChange) {
            DrawCall newDrawCall = {
                .indexOffset = baseIndex,
                .indexCount = 0,
                .blendMode = blendMode,
                .shader = shader,
                .textureUnits = {},
                .textureUnitCount = 0,
            };

            drawCalls.push_back(newDrawCall);
            return drawCalls.back();
        }

        return *previousDrawCall;
    }

    static uint32_t resolve_texture_unit(DrawCall& drawCall, uint32_t texture)
    {
        for (size_t i = 0; i < drawCall.textureUnitCount; ++i) {
            if (drawCall.textureUnits[i] == texture) {
                return i;
            }
        }

        assert((drawCall.textureUnitCount < drawCall.textureUnits.size()) and "Exceeded maximum texture units per draw call");
        const size_t newTextureUnitIndex = drawCall.textureUnitCount;
        drawCall.textureUnits[newTextureUnitIndex] = texture;
        drawCall.textureUnitCount++;
        return newTextureUnitIndex;
    }

    void draw_calls_submit(DrawScope& scope, DrawCallList& drawCalls, std::shared_ptr<Shader> shader, BlendMode blendMode, uint32_t texture)
    {
        const uint32_t vertexCount = scope.vertexCursor - scope.baseVertex;
        const uint32_t indexCount = scope.indexCursor - scope.baseIndex;

        DrawCall& drawCall = get_or_create_draw_call(drawCalls, std::move(shader), blendMode, texture, scope.baseIndex);
        drawCall.indexCount += indexCount;

        const size_t textureUnitIndex = resolve_texture_unit(drawCall, texture);
        for (size_t i = 0; i < vertexCount; ++i) {
            scope.vertices[scope.baseVertex + i].texIndex = static_cast<float>(textureUnitIndex);
        }
    }
} // namespace p5
