#pragma once

#include "drawscope.hpp"
#include "p5.hpp"
#include "renderstate.hpp"
#include <unordered_map>

namespace p5
{
    // Shader uniforms must be implemented in a way that they can be set by the user.
    // If they are set multiple times per frame, we need to make sure that these draw calls
    // are not being batched together.
    //
    // Example:
    //
    // shader(...);
    // setUniform("u_Time", uniform(time));
    // rect(...);
    // noShader();
    // ...
    // shader(...);
    // setUniform("u_Time", uniform(time) * 0.5f);
    // rect(...);
    // noShader();
    //
    // The two rect calls above cannot be batched together, because they use different uniform values. If we batch them together,
    // we would need to use the same uniform value for both draw calls, which is not what the user expects.
    //
    // Additionally the last call to setUniform in a frame where setUniform is called multiple times before actually submitting a draw-call via rect() for example
    // should win.
    //
    // Example:
    // shader(...);
    // setUniform("u_Time", uniform(time));
    // setUniform("u_Time", uniform(time) * 0.5f);
    // rect(...);
    // noShader();
    //
    // Besides that, Calling setUniform multiple times with the same uniform name and value on the same exact
    // shader should not cause draw calls to be unbatched, because the uniform value is the same and does not cause any visual difference.
    // Example:
    // shader(...);
    // setUniform("u_Time", uniform(time));
    // rect(...);
    // setUniform("u_Time", uniform(time)); // This should not cause the previous rect call to be unbatched, because the uniform value is the same.
    // rect(...);
    // noShader();

    struct CachedUniformVariable
    {
        bool dirty;            // A flag to indicate whether the uniform variable has been changed since the last time it was set, so that we can avoid setting the same uniform variable multiple times if it has not changed.
        int location;          // The internal location of the uniform variable in the shader, so that we can avoid calling glGetUniformLocation multiple times for the same uniform variable.
        UniformVariable value; // The cached value of the uniform variable, so that we can compare it with the new value when the user calls setUniform, to determine whether the uniform variable has actually changed and needs to be updated in the shader.
    };

    // An entry represents an entire cache for a specific shader.
    struct UniformCacheEntry
    {
        std::unordered_map<std::string, CachedUniformVariable> variables; // A map of uniform variable names to their cached values, so that we can quickly look up the cached value for a specific uniform variable when we need to set it.
    };

    void uniform_cache_entry_set(UniformCacheEntry& entry, std::string_view name, const UniformVariable& variable);

    struct UniformCache
    {
        std::unordered_map<Shader*, UniformCacheEntry> shaderUniforms;
    };

    UniformCache uniform_cache_create();
    bool uniform_cache_is_dirty(const UniformCache& cache, std::shared_ptr<Shader> shader);
    UniformCacheEntry& uniform_cache_get_entry(UniformCache& cache, std::shared_ptr<Shader> shader);

    // A Draw-Command represents a single draw call made by the user, such as
    // drawing a rectangle, a line, or a text. It captures all the necessary information
    // for the renderer to execute this draw call, such as the shader to use, the blend mode as well as the textures and more.
    struct DrawCommand
    {
        size_t drawBufferIndexStart;          // Where in the global DrawBuffer this draw command starts
        size_t drawBufferIndexCount;          // How many indices this draw command has in the global DrawBuffer
        std::shared_ptr<Shader> shader;       // Which shader to use for this draw command
        UniformCacheEntry* uniformCacheEntry; // A pointer to the uniform cache entry for the shader used by this draw command, so that we can check if any uniform variables used by this draw command have been changed since the last time they were set, to determine whether we can merge this draw command with previous draw commands that use the same shader.
        BlendMode blendMode;                  // Which blend mode to use for this draw command
        std::array<uint32_t, 8> textureUnits; // Which texture units to bind for this draw command
        size_t textureUnitCount;              // The number of texture units used by this draw command
    };

    // The DrawCommandList is a wrapper around a list of DrawCommands, which is used by the renderer to execute the draw calls in the correct order.
    // Additionally, we can make sure that draw commands are being merged together when possible, for example when they use the same shader and blend mode, to reduce the number of draw calls and improve performance.
    typedef std::vector<DrawCommand> DrawCommandList;

    // This function is responsible for submitting the draw commands to the renderer, it takes care of merging draw commands together when possible, and also resolves texture units for each draw command.
    void draw_commands_submit(DrawCommandList& commands, UniformCache& cache, const DrawScope& scope, std::shared_ptr<Shader> shader, BlendMode blendMode, uint32_t texture);

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
