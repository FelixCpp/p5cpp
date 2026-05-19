#include "canvas.hpp"
#include "p5.hpp"
#include <cstdio>
#include <optional>

namespace p5
{
    void uniform_cache_entry_set(UniformCacheEntry& entry, std::string_view name, const UniformVariable& variable)
    {
        std::string key(name);
        const auto itr = entry.variables.find(key);
        if (itr != entry.variables.end()) {
            // For simplicity we do not compare the values for now but instead mark the uniform variable
            // as dirty whenever it is set, even if the value is the same as the previous value.
            // This means that we will be setting the same uniform variable multiple times in a frame if
            // the user calls setUniform multiple times with the same uniform name and value, which is not
            // optimal but it is correct and does not cause any visual difference, so we can optimize this
            // later if needed.
            CachedUniformVariable& variable = itr->second;
            variable.dirty = true;
            return;
        }

        // At this point the uniform variable is not yet in the cache, so we need to add it to the cache and mark it as dirty.
        entry.variables.insert(
            std::make_pair(
                std::move(key),
                CachedUniformVariable {
                    .dirty = true,
                    .location = -1, // We will get the actual location of the uniform variable in the shader when we need to set it, so we can avoid calling glGetUniformLocation multiple times for the same uniform variable.
                    .value = variable,
                }
            )
        );
    }

    UniformCache uniform_cache_create()
    {
        return UniformCache {
            .shaderUniforms = {}
        };
    }

    bool uniform_cache_is_dirty(const UniformCache& cache, std::shared_ptr<Shader> shader)
    {
        const auto itr = cache.shaderUniforms.find(shader.get());
        if (itr == cache.shaderUniforms.end()) {
            return false;
        }

        const UniformCacheEntry& entry = itr->second;
        for (const auto& pair : entry.variables) {
            const CachedUniformVariable& variable = pair.second;
            if (variable.dirty) {
                return true;
            }
        }

        return false;
    }

    UniformCacheEntry& uniform_cache_get_entry(UniformCache& cache, std::shared_ptr<Shader> shader)
    {
        const auto itr = cache.shaderUniforms.find(shader.get());
        if (itr != cache.shaderUniforms.end()) {
            return itr->second;
        }

        // At this point there is no cache entry for this shader yet, so we need to create a new cache entry for this shader and add it to the cache.
        UniformCacheEntry& entry = cache.shaderUniforms[shader.get()];
        entry.variables = {};
        return entry;
    }
} // namespace p5

namespace p5
{
    inline static bool draw_command_mergeable(const DrawCommand& command, UniformCache& uniformCache, std::shared_ptr<Shader> shader, BlendMode blendMode, uint32_t texture)
    {
        // We can not merge draw commands if they use different shaders.
        if (command.shader != shader) {
            return false;
        }

        // DrawCommands are not mergeable if the uniform cache for the shader is dirty, because that means that there
        // are uniform variables that have been changed since the last time they were set, and we need to set those
        // uniform variables before rendering this draw command, which means that we can not merge this draw command
        // with any previous draw commands that use the same shader but have a different uniform variable state.
        if (uniform_cache_is_dirty(uniformCache, shader)) {
            return false;
        }

        // We can not merge draw commands if they use different blend modes.
        if (command.blendMode != blendMode) {
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

    inline static DrawCommand& create_draw_command(DrawCommandList& commands, UniformCache& uniformCache, const DrawScope& scope, std::shared_ptr<Shader> shader, BlendMode blendMode, uint32_t texture)
    {
        return commands.emplace_back(DrawCommand {
            .drawBufferIndexStart = scope.baseIndex,
            .drawBufferIndexCount = 0,
            .shader = std::move(shader),
            .uniformCacheEntry = &uniform_cache_get_entry(uniformCache, shader),
            .blendMode = blendMode,
            .textureUnits = {texture},
            .textureUnitCount = 1,
        });
    }

    inline static DrawCommand& draw_commands_get_or_create(DrawCommandList& commands, UniformCache& cache, const DrawScope& scope, std::shared_ptr<Shader> shader, BlendMode blendMode, uint32_t texture)
    {
        if (not commands.empty() and draw_command_mergeable(commands.back(), cache, shader, blendMode, texture)) {
            return commands.back();
        }

        return create_draw_command(commands, cache, scope, shader, blendMode, texture);
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
