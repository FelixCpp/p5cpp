#pragma once

#include "p5.hpp"
#include "renderstate.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

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

namespace p5
{
    struct RenderPass
    {
        std::shared_ptr<Canvas> canvas;
        std::vector<DrawCall> drawCalls;
        RenderStateStack renderStates;
    };
} // namespace p5

namespace p5
{
    class RenderPassStack
    {
    public:
        RenderPassStack(std::shared_ptr<Canvas> defaultCanvas);

        void push(std::shared_ptr<Canvas> canvas);
        void pop();
        void reset();

        RenderPass& peek();

        std::span<RenderPass> getRenderPasses();

    private:
        std::vector<RenderPass> m_renderPasses;
        std::shared_ptr<Canvas> m_defaultCanvas;
        size_t m_activeRenderPassIndex;
    };
} // namespace p5
