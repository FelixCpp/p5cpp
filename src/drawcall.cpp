#include "drawcall.hpp"

#include <cassert>

namespace p5
{
    static DrawCall& get_or_create_draw_call(DrawCallList& drawCalls, std::shared_ptr<Shader> shader, std::vector<UniformVariable>& uniforms, bool& uniformsDirty, BlendMode blendMode, uint32_t texture, uint32_t baseIndex)
    {
        DrawCall* previousDrawCall = drawCalls.empty() ? nullptr : &drawCalls.back();

        const bool hasShaderChange = previousDrawCall != nullptr and previousDrawCall->shader != shader;
        const bool hasBlendModeChange = previousDrawCall != nullptr and previousDrawCall->blendMode != blendMode;

        const bool hasTextureChange = previousDrawCall != nullptr and std::invoke([&]() {
                                          for (size_t i = 0; i < previousDrawCall->textureUnitCount; ++i) {
                                              if (previousDrawCall->textureUnits[i] == texture) {
                                                  return false;
                                              }
                                          }
                                          return true;
                                      });

        const bool isTextureSlotAvailable = previousDrawCall != nullptr and previousDrawCall->textureUnitCount < previousDrawCall->textureUnits.size();
        const bool canBatchWithPrevious = !hasShaderChange and !hasBlendModeChange and (not hasTextureChange or isTextureSlotAvailable) and not uniformsDirty;

        if (previousDrawCall == nullptr or not canBatchWithPrevious) {
            DrawCall newDrawCall = {
                .indexOffset = baseIndex,
                .indexCount = 0,
                .blendMode = blendMode,
                .shader = shader,
                .shaderUniforms = std::vector<UniformVariable>(uniforms.begin(), uniforms.end()),
                .textureUnits = {},
                .textureUnitCount = 0,
            };

            uniforms.clear();
            uniformsDirty = false;

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

    void draw_calls_submit(DrawScope& scope, DrawCallList& drawCalls, std::shared_ptr<Shader> shader, std::vector<UniformVariable>& uniforms, bool& uniformsDirty, BlendMode blendMode, uint32_t texture)
    {
        const uint32_t vertexCount = scope.vertexCursor - scope.baseVertex;
        const uint32_t indexCount = scope.indexCursor - scope.baseIndex;

        DrawCall& drawCall = get_or_create_draw_call(drawCalls, std::move(shader), uniforms, uniformsDirty, blendMode, texture, scope.baseIndex);
        drawCall.indexCount += indexCount;

        const size_t textureUnitIndex = resolve_texture_unit(drawCall, texture);
        for (size_t i = 0; i < vertexCount; ++i) {
            scope.vertices[scope.baseVertex + i].texIndex = static_cast<float>(textureUnitIndex);
        }
    }
} // namespace p5
