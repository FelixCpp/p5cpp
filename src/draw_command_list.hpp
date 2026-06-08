#pragma once

#include "draw_command.hpp"
#include "draw_scope.hpp"
#include "uniform_cache.hpp"

namespace p5
{
    // The DrawCommandList is a wrapper around a list of DrawCommands, which is used by the renderer to execute the draw calls in the correct order.
    // Additionally, we can make sure that draw commands are being merged together when possible, for example when they use the same shader and blend mode, to reduce the number of draw calls and improve performance.
    typedef std::vector<DrawCommand> DrawCommandList;

    // This function is responsible for submitting the draw commands to the renderer, it takes care of merging draw commands together when possible, and also resolves texture units for each draw command.
    void draw_commands_submit(DrawCommandList& drawCommands, UniformCache& uniformCache, const DrawScope& scope, std::shared_ptr<Shader> shader, BlendMode blendMode, uint32_t texture);
    void draw_commands_clear(DrawCommandList& drawCommands);
} // namespace p5
