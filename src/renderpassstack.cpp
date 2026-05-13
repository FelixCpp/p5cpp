#include "renderpassstack.hpp"

namespace p5
{
    RenderPassStack render_passes_create(std::shared_ptr<Canvas> initialCanvas)
    {
        return RenderPassStack {
            .renderPasses = {
                RenderPass {
                    .canvas = std::move(initialCanvas),
                    .drawCalls = {}
                }
            },
            .activeRenderPassIndex = 0
        };
    }

    void render_passes_push(RenderPassStack& stack, std::shared_ptr<Canvas> canvas)
    {
        stack.renderPasses.push_back(
            RenderPass {
                .canvas = std::move(canvas),
                .drawCalls = {}
            }
        );

        stack.activeRenderPassIndex = stack.renderPasses.size() - 1;
    }

    void render_passes_pop(RenderPassStack& stack)
    {
        if (stack.activeRenderPassIndex > 0) {
            --stack.activeRenderPassIndex;
        }
    }

    void render_passes_clear(RenderPassStack& stack)
    {
        stack.renderPasses.clear();
        stack.activeRenderPassIndex = 0;
    }

    const RenderPass& render_passes_peek(const RenderPassStack& stack)
    {
        return stack.renderPasses.at(stack.activeRenderPassIndex);
    }

    RenderPass& render_passes_peek(RenderPassStack& stack)
    {
        return stack.renderPasses.at(stack.activeRenderPassIndex);
    }

    uint2 render_passes_get_active_canvas_size(const RenderPassStack& stack)
    {
        return render_passes_peek(stack).canvas->getSize();
    }
} // namespace p5
