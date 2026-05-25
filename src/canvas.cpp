#include "canvas.hpp"
#include "p5.hpp"
#include <cstdio>
#include <optional>

namespace p5
{
    inline static bool draw_command_mergeable(const DrawCommand& command, const std::shared_ptr<Shader>& shader, ShaderUniformCache& cache, BlendMode blendMode, uint32_t texture)
    {
        // We can not merge draw commands if they use different shaders.
        if (command.shader != shader) {
            return false;
        }

        // We can not merge draw commands if they use different blend modes.
        if (command.blendMode != blendMode) {
            return false;
        }

        if (not cache.dirtyUniformIndices.empty()) {
            return false;
        }

        // We can not merge draw commands if they use different textures, unless one of the draw commands has an available texture slot for the new texture.
        const bool isTextureAlreadyUsed = std::invoke([&command, texture]() {
            for (size_t i = 0; i < command.textureUnitCount; ++i) {
                if (command.textureUnits[i] == texture) {
                    return true;
                }
            }
            return false;
        });

        const bool isTextureSlotAvailable = command.textureUnitCount < command.textureUnits.size();
        if (not isTextureAlreadyUsed and not isTextureSlotAvailable) {
            return false;
        }

        // We are actually able to merge these draw commands together, either because they use the same texture, or because one of the draw commands has an available texture slot for the new texture.
        return true;
    }

    inline static DrawCommand& create_draw_command(DrawCommandList& commands, const DrawScope& scope, std::shared_ptr<Shader> shader, ShaderUniformCache& cache, BlendMode blendMode, uint32_t texture)
    {
        return commands.emplace_back(
            DrawCommand {
                .drawBufferIndexStart = scope.baseIndex,
                .drawBufferIndexCount = 0,
                .shader = std::move(shader),
                .uniforms = {},
                .blendMode = blendMode,
                .textureUnits = {texture},
                .textureUnitCount = 1,
            }
        );
    }

    inline static DrawCommand& draw_commands_get_or_create(DrawCommandList& commands, ShaderUniformCache& shaderCache, const DrawScope& scope, std::shared_ptr<Shader> shader, BlendMode blendMode, uint32_t texture)
    {
        if (not commands.empty() and draw_command_mergeable(commands.back(), shader, shaderCache, blendMode, texture)) {
            return commands.back();
        }

        return create_draw_command(commands, scope, shader, shaderCache, blendMode, texture);
    }

    inline static std::optional<size_t> find_texture_unit_index(const DrawCommand& command, uint32_t texture)
    {
        for (size_t i = 0; i < command.textureUnitCount; ++i) {
            if (command.textureUnits[i] == texture) {
                return i;
            }
        }

        return std::nullopt;
    }

    void draw_commands_submit(DrawCommandList& commands, UniformCache& cache, const DrawScope& scope, std::shared_ptr<Shader> shader, BlendMode blendMode, uint32_t texture)
    {
        ShaderUniformCache& shaderCache = uniform_cache_get_shader_cache(cache, shader.get());

        // Get or create a draw command for this draw call, and update its draw buffer index count to include the new indices from this draw call.
        DrawCommand& command = draw_commands_get_or_create(commands, shaderCache, scope, shader, blendMode, texture);
        command.drawBufferIndexCount += scope.indexCursor - scope.baseIndex;

        {
            std::vector<LocatedUniform> newUniforms;
            for (const size_t id : shaderCache.dirtyUniformIndices) {
                const UniformVariable& variable = shader_uniform_cache_get_variable(shaderCache, id);
                const int location = shader_uniform_cache_get_location(shaderCache, id, [&shader](const std::string& name) {
                    return shader->getUniformLocation(name);
                });

                newUniforms.push_back(LocatedUniform {
                    .location = location,
                    .variable = variable,
                });
            }

            command.uniforms.append_range(std::move(newUniforms));
            shader_uniform_cache_mark_upload(shaderCache, shaderCache.dirtyUniformIndices);
        }

        // Now we need to make sure that the texture used by this draw command is included in the command's texture units, if it is not already included.
        // To archive this we need to find an available texture unit for this texture, and add it to the command's texture units if it is not already included.
        // Then we need to check if we found the texture in the command's texture units, or if we need to add it to the command's texture units.
        const std::optional<size_t> textureUnitIndex = find_texture_unit_index(command, texture);
        size_t newTextureUnitIndex;
        if (not textureUnitIndex.has_value()) {
            newTextureUnitIndex = command.textureUnitCount;
            command.textureUnits[newTextureUnitIndex] = texture;
            command.textureUnitCount++;
        } else {
            newTextureUnitIndex = textureUnitIndex.value();
        }

        // We must update the "texIndex" attribute of the vertices in the draw scope to point to the correct texture unit index for this texture, so that
        // the shader can sample from the correct texture unit when rendering this draw command.
        const size_t vertexCount = scope.vertexCursor - scope.baseVertex;
        for (size_t i = 0; i < vertexCount; ++i) {
            scope.vertices[scope.baseVertex + i].texIndex = static_cast<float>(newTextureUnitIndex);
        }
    }
} // namespace p5

namespace p5
{
    void canvas_stack_push(CanvasStack& stack, const Canvas& canvas)
    {
        stack.push_back(canvas);
    }

    void canvas_stack_pop(CanvasStack& stack)
    {
        if (not stack.empty()) {
            stack.pop_back();
        }
    }

    void canvas_stack_clear(CanvasStack& stack)
    {
        stack.clear();
    }

    bool canvas_stack_is_empty(const CanvasStack& stack)
    {
        return stack.empty();
    }

    Canvas& canvas_stack_peek(CanvasStack& stack)
    {
        return stack.back();
    }
} // namespace p5
