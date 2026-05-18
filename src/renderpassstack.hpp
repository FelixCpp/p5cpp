#pragma once

#include "p5.hpp"
#include "drawcall.hpp"

#include <vector>

namespace p5
{
    struct RenderPass
    {
        std::shared_ptr<Canvas> canvas;
        DrawCallList drawCalls;
    };

    struct ActiveRenderPassIndex
    {
        size_t activeIndex;
        size_t continuationIndex;
    };

    struct RenderPassStack
    {
        std::vector<RenderPass> renderPasses;
        std::vector<ActiveRenderPassIndex> activeStack;
    };

    RenderPassStack render_passes_create();
    void render_passes_begin_frame(RenderPassStack& stack, std::shared_ptr<Canvas> initialCanvas);
    void render_passes_push(RenderPassStack& stack, std::shared_ptr<Canvas> canvas);
    void render_passes_pop(RenderPassStack& stack);
    void render_passes_clear(RenderPassStack& stack);

    const RenderPass& render_passes_peek(const RenderPassStack& stack);
    RenderPass& render_passes_peek(RenderPassStack& stack);

    uint2 render_passes_get_active_canvas_size(const RenderPassStack& stack);
} // namespace p5
