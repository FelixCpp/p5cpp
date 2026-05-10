#include "renderpass.hpp"

namespace p5
{
    RenderPassStack::RenderPassStack(std::shared_ptr<Canvas> defaultCanvas)
        : m_renderPasses(), m_defaultCanvas(std::move(defaultCanvas)), m_activeRenderPassIndex(0)
    {
        m_renderPasses.push_back(RenderPass {
            .canvas = m_defaultCanvas,
            .drawCalls = {},
            .renderStates = RenderStateStack(),
        });
    }

    void RenderPassStack::push(std::shared_ptr<Canvas> canvas)
    {
        m_renderPasses.push_back({canvas});
        m_activeRenderPassIndex = m_renderPasses.size() - 1;
    }

    void RenderPassStack::pop()
    {
        if (m_renderPasses.empty())
            return;

        --m_activeRenderPassIndex;
    }

    void RenderPassStack::reset()
    {
        m_renderPasses.clear();
        m_renderPasses.push_back(RenderPass {
            .canvas = m_defaultCanvas,
            .drawCalls = {},
            .renderStates = RenderStateStack(),
        });

        m_activeRenderPassIndex = 0;
    }

    RenderPass& RenderPassStack::peek()
    {
        return m_renderPasses.at(m_activeRenderPassIndex);
    }

    std::span<RenderPass> RenderPassStack::getRenderPasses()
    {
        return std::span<RenderPass>(m_renderPasses.data(), m_renderPasses.size());
    }
} // namespace p5
