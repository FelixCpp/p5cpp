#pragma once

#include "drawscope.hpp"
#include "p5.hpp"
#include "renderstate.hpp"

namespace p5
{
    // A Draw-Command represents a single draw call made by the user, such as
    // drawing a rectangle, a line, or a text. It captures all the necessary information
    // for the renderer to execute this draw call, such as the shader to use, the blend mode as well as the textures and more.
    struct DrawCommand
    {
        size_t drawBufferIndexStart;                      // Where in the global DrawBuffer this draw command starts
        size_t drawBufferIndexCount;                      // How many indices this draw command has in the global DrawBuffer
        std::shared_ptr<Shader> shader;                   // Which shader to use for this draw command
        std::vector<NamedUniformVariable> shaderUniforms; // Which shader uniforms to use for this draw command, as well as their values, used to determine if two draw commands can be merged together or not, as we can only merge draw commands together if they use the same shader uniforms as well.
        size_t shaderUniformHash;                         // A hash of the shader uniforms used by this draw command, used to determine if two draw commands can be merged together or not, as we can only merge draw commands together if they use the same shader uniforms as well.
        BlendMode blendMode;                              // Which blend mode to use for this draw command
        std::array<uint32_t, 8> textureUnits;             // Which texture units to bind for this draw command
        size_t textureUnitCount;                          // The number of texture units used by this draw command
    };

    // The DrawCommandList is a wrapper around a list of DrawCommands, which is used by the renderer to execute the draw calls in the correct order.
    // Additionally, we can make sure that draw commands are being merged together when possible, for example when they use the same shader and blend mode, to reduce the number of draw calls and improve performance.
    typedef std::vector<DrawCommand> DrawCommandList;

    // This function is responsible for submitting the draw commands to the renderer, it takes care of merging draw commands together when possible, and also resolves texture units for each draw command.
    void draw_commands_submit(DrawCommandList& commands, const DrawScope& scope, std::shared_ptr<Shader> shader, BlendMode blendMode, uint32_t texture);

    // A canvas consists out of a framebuffer to which we can render,
    // as well as a list of render states controlled by the user.
    //
    // Note that we also store a list of draw commands for each canvas.
    // This forces us to flush the rnederer every time the user switches between canvases.
    // Theoretically, we could avoid this by keeping a global list of draw commands, but this would make it
    // harder to manage the draw commands and their associated render states, as well as making it harder to
    // implement features such as render state stacks and more complex rendering techniques.
    struct Canvas
    {
        std::shared_ptr<Framebuffer> framebuffer;
        RenderStateStack renderStates;
        DrawCommandList drawCommands;
    };

    // Temporarily, we can use a simple vector to manage the canvas stack, but in the future we might want to
    // implement a more complex data structure to manage the canvas stack, such as a linked list or a tree,
    // to allow for more complex rendering techniques and optimizations.
    typedef std::vector<Canvas> CanvasStack;

    void canvas_stack_push(CanvasStack& stack, const Canvas& canvas);
    void canvas_stack_pop(CanvasStack& stack);
    void canvas_stack_clear(CanvasStack& stack);
    bool canvas_stack_is_empty(const CanvasStack& stack);
    Canvas& canvas_stack_peek(CanvasStack& stack);
} // namespace p5
