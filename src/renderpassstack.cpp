#include "renderpassstack.hpp"

namespace p5
{
    RenderPassStack render_passes_create()
    {
        return RenderPassStack {
            .renderPasses = {},
            .activeStack = {}
        };
    }

    void render_passes_begin_frame(RenderPassStack& stack, std::shared_ptr<Canvas> initialCanvas)
    {
        stack.renderPasses.clear();
        stack.activeStack.clear();

        stack.renderPasses.push_back(RenderPass {
            .canvas = std::move(initialCanvas),
            .drawCalls = {},
        });

        stack.activeStack.push_back(ActiveRenderPassIndex {
            .activeIndex = 0,
            .continuationIndex = 0,
        });
    }

    void render_passes_push(RenderPassStack& stack, std::shared_ptr<Canvas> canvas)
    {
        const size_t currentIndex = stack.activeStack.back().activeIndex;

        stack.renderPasses.insert(
            stack.renderPasses.begin() + currentIndex + 1,
            RenderPass {
                .canvas = stack.renderPasses[currentIndex].canvas,
                .drawCalls = {},
            } // continuation
        );

        stack.renderPasses.insert(
            stack.renderPasses.begin() + currentIndex + 1,
            RenderPass {
                .canvas = std::move(canvas),
                .drawCalls = {},
            } // neuer canvas
        );

        for (ActiveRenderPassIndex& entry : stack.activeStack) {
            if (entry.activeIndex > currentIndex) {
                entry.activeIndex += 2;
            }
            if (entry.continuationIndex > currentIndex) {
                entry.continuationIndex += 2;
            }
        }

        stack.activeStack.push_back(ActiveRenderPassIndex {
            .activeIndex = currentIndex + 1,
            .continuationIndex = currentIndex + 2,
        });
    }

    void render_passes_pop(RenderPassStack& stack)
    {
        if (stack.activeStack.size() > 1) {
            const size_t continuationIndex = stack.activeStack.back().continuationIndex;
            const size_t parentContinuation = stack.activeStack[stack.activeStack.size() - 2].continuationIndex;
            stack.activeStack.pop_back();
            stack.activeStack.back() = ActiveRenderPassIndex {
                .activeIndex = continuationIndex,
                .continuationIndex = parentContinuation,
            };
        }
    }

    void render_passes_clear(RenderPassStack& stack)
    {
        stack.renderPasses.clear();
        stack.activeStack.clear();
    }

    const RenderPass& render_passes_peek(const RenderPassStack& stack)
    {
        return stack.renderPasses.at(stack.activeStack.back().activeIndex);
    }

    RenderPass& render_passes_peek(RenderPassStack& stack)
    {
        return stack.renderPasses.at(stack.activeStack.back().activeIndex);
    }

    uint2 render_passes_get_active_canvas_size(const RenderPassStack& stack)
    {
        return render_passes_peek(stack).canvas->getSize();
    }
} // namespace p5
