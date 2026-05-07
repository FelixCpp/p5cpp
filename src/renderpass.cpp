#include "renderpass.hpp"
#include "meshwriter.hpp"

#include <optional>

namespace p5
{
    void renderpass_submit(RenderPass& pass, MeshWriter& writer, uint32_t texture, std::shared_ptr<Shader> shader, BlendMode blendMode)
    {
        if (pass.drawCalls.empty()) {
            DrawCall drawCall = {
                .indexOffset = writer.getBaseIndex(),
                .indexCount = 0,
                .blendMode = blendMode,
                .shader = shader,
                .textureUnits = {},
                .textureUnitCount = 0,
            };

            pass.drawCalls.push_back(drawCall);
        } else {
            DrawCall& drawCall = pass.drawCalls.back();
            const bool hasShaderChange = drawCall.shader != shader;
            const bool hasBlendModeChange = drawCall.blendMode != blendMode;
            const bool hasTextureChange = std::invoke([&]() {
                for (size_t i = 0; i < drawCall.textureUnitCount; ++i) {
                    if (drawCall.textureUnits[i] == texture) {
                        return false;
                    }
                }

                return true;
            });
            const bool textureSlotAvailable = drawCall.textureUnitCount < drawCall.textureUnits.size();
            const bool canBatch = not hasShaderChange and not hasBlendModeChange and (not hasTextureChange or textureSlotAvailable);

            if (not canBatch) {
                DrawCall newDrawCall = {
                    .indexOffset = writer.getBaseIndex(),
                    .indexCount = 0,
                    .blendMode = blendMode,
                    .shader = shader,
                    .textureUnits = {},
                    .textureUnitCount = 0,
                };

                pass.drawCalls.push_back(newDrawCall);
            }
        }

        DrawCall& drawCall = pass.drawCalls.back();
        drawCall.indexCount += writer.getIndexCount();

        std::optional<size_t> foundTextureUnitIndex;
        for (size_t i = 0; i < drawCall.textureUnitCount; ++i) {
            if (drawCall.textureUnits[i] == texture) {
                foundTextureUnitIndex = i;
                break;
            }
        }

        const size_t textureUnitIndex = foundTextureUnitIndex.value_or(drawCall.textureUnitCount);

        if (not foundTextureUnitIndex.has_value()) {
            drawCall.textureUnits[textureUnitIndex] = texture;
            drawCall.textureUnitCount++;
        }

        writer.setTextureIndex(static_cast<float>(textureUnitIndex));
    }
} // namespace p5

namespace p5
{
    RenderPassStack::RenderPassStack(std::shared_ptr<Canvas> defaultCanvas)
        : m_currentRenderPassIndex(0),
          m_defaultRenderPass(RenderPass {
              .canvas = defaultCanvas,
              .drawCalls = {},
          })
    {
    }

    void RenderPassStack::activate(std::shared_ptr<Canvas> canvas)
    {
        for (size_t i = 0; i < m_renderPasses.size(); ++i) {
            if (m_renderPasses[i].canvas == canvas) {
                m_currentRenderPassIndex = i;
                return;
            }
        }

        m_renderPasses.push_back(RenderPass {
            .canvas = canvas,
            .drawCalls = {},
        });

        m_currentRenderPassIndex = m_renderPasses.size() - 1;
    }

    std::shared_ptr<Canvas> RenderPassStack::getDefaultCanvas() const
    {
        return m_defaultRenderPass.canvas;
    }

    RenderPass* RenderPassStack::getCurrentRenderPass()
    {
        if (m_renderPasses.empty()) {
            return &m_defaultRenderPass;
        }

        return &m_renderPasses[m_currentRenderPassIndex];
    }

    void RenderPassStack::clear()
    {
        m_renderPasses.clear();
        m_currentRenderPassIndex = 0;
    }

    std::span<const RenderPass> RenderPassStack::getRenderPasses() const
    {
        if (m_renderPasses.empty()) {
            return std::span {&m_defaultRenderPass, 1};
        }

        return std::span {m_renderPasses.data(), m_renderPasses.size()};
    }
} // namespace p5
