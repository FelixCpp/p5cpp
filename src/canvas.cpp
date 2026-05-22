#include "canvas.hpp"
#include "p5.hpp"
#include <cstdio>
#include <optional>

namespace p5
{
    void shader_uniform_cache_set(ShaderUniformCache& cache, std::string_view name, const UniformVariable& variable)
    {
        for (UniformCacheEntry& entry : cache.entries) {
            if (entry.name == name) {
                // std::fprintf(stdout, "Found Uniform '%.*s' in cache\n", static_cast<int>(name.size()), name.data());
                // std::fflush(stdout);

                entry.variable = variable;
                entry.dirty = true;
                return;
            }
        }

        std::fprintf(stdout, "Pushed Uniform '%.*s' to cache\n", static_cast<int>(name.size()), name.data());
        std::fflush(stdout);
        cache.entries.push_back(
            UniformCacheEntry {
                .name = std::string(name),
                .dirty = true,
                .location = std::nullopt,
                .variable = variable,
            }
        );
    }

    void shader_uniform_cache_mark_upload(ShaderUniformCache& cache, std::string_view name)
    {
        for (UniformCacheEntry& entry : cache.entries) {
            if (entry.name == name) {
                entry.dirty = false;
                return;
            }
        }
    }
} // namespace p5

namespace p5
{
    UniformCache uniform_cache_create()
    {
        return UniformCache {
            .entries = {},
        };
    }

    ShaderUniformCache& uniform_cache_get_shader_cache(UniformCache& cache, Shader* shader)
    {
        const auto itr = cache.entries.find(shader);
        if (itr != cache.entries.end()) {
            return itr->second;
        }

        // If there is no cache for this shader, we create an empty one and return it.
        return cache.entries.insert(std::make_pair(shader, ShaderUniformCache {})).first->second;
    }
} // namespace p5

namespace p5
{
    inline static bool draw_command_mergeable(const DrawCommand& command, ShaderUniformCache& cache, std::shared_ptr<Shader> shader, BlendMode blendMode, uint32_t texture)
    {
        // We can not merge draw commands if they use different shaders.
        if (command.shader != shader) {
            return false;
        }

        // We can not merge draw commands if they use different blend modes.
        if (command.blendMode != blendMode) {
            return false;
        }

        if (command.uniformCache.size() != cache.entries.size()) {
            return false;
        }

        // Check if the uniforms have changed between these draw commands, if they have
        // changed we can not merge these draw commands together, as they would require
        // different uniform values to be uploaded when the renderer executes them.
        for (const UniformCacheEntry& entry : command.uniformCache) {
            if (entry.dirty) {
                return false;
            }
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

    inline static DrawCommand& create_draw_command(DrawCommandList& commands, ShaderUniformCache& cache, const DrawScope& scope, std::shared_ptr<Shader> shader, BlendMode blendMode, uint32_t texture)
    {
        return commands.emplace_back(
            DrawCommand {
                .drawBufferIndexStart = scope.baseIndex,
                .drawBufferIndexCount = 0,
                .shader = std::move(shader),
                .uniformCache = cache.entries,
                .blendMode = blendMode,
                .textureUnits = {texture},
                .textureUnitCount = 1,
            }
        );
    }

    inline static DrawCommand& draw_commands_get_or_create(DrawCommandList& commands, UniformCache& cache, const DrawScope& scope, std::shared_ptr<Shader> shader, BlendMode blendMode, uint32_t texture)
    {
        ShaderUniformCache& shaderCache = uniform_cache_get_shader_cache(cache, shader.get());

        if (not commands.empty() and draw_command_mergeable(commands.back(), shaderCache, shader, blendMode, texture)) {
            return commands.back();
        }

        return create_draw_command(commands, shaderCache, scope, shader, blendMode, texture);
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
        // Get or create a draw command for this draw call, and update its draw buffer index count to include the new indices from this draw call.
        DrawCommand& command = draw_commands_get_or_create(commands, cache, scope, shader, blendMode, texture);
        command.drawBufferIndexCount += scope.indexCursor - scope.baseIndex;

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
