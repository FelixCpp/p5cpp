#pragma once

#include "p5.hpp"

#include <vector>
#include <cstdint>
#include <memory>

namespace p5
{

    class MeshWriter;

    struct DrawCall
    {
        uint32_t indexOffset;
        uint32_t indexCount;

        BlendMode blendMode;

        std::shared_ptr<Shader> shader;

        std::array<uint32_t, 8> textureUnits;
        size_t textureUnitCount;
    };

    struct RenderPass
    {
        std::shared_ptr<Canvas> canvas;
        std::vector<DrawCall> drawCalls;
    };

    class RenderPassStack
    {
    public:
        RenderPassStack(std::shared_ptr<Canvas> defaultCanvas);
        void activate(std::shared_ptr<Canvas> canvas);
        std::shared_ptr<Canvas> getDefaultCanvas() const;
        RenderPass* getCurrentRenderPass();
        void clear();

        std::span<const RenderPass> getRenderPasses() const;

    private:
        std::vector<RenderPass> m_renderPasses;
        size_t m_currentRenderPassIndex;
        RenderPass m_defaultRenderPass;
    };

    void renderpass_submit(RenderPass& pass, MeshWriter& writer, uint32_t texture, std::shared_ptr<Shader> shader, BlendMode blendMode);
} // namespace p5
