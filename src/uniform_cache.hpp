#pragma once

#include "p5.hpp"

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <functional>

namespace p5
{
    using UniformIds = std::unordered_set<size_t>;

    // This struct represents a a cache for a single shader.
    struct ShaderUniformCache
    {
        std::vector<UniformVariable> uniforms;                        // A list of uniform variables that have been set by the user.
        UniformIds dirtyUniformIndices;                               // A set of indices of the uniform variables that have been modified since the last time they were uploaded to the GPU, this is used to optimize the uniform uploads by only uploading the modified uniforms.
        std::unordered_map<size_t, int> uniformLocationCache;         // A cache of uniform locations for this shader, this allows us to quickly access the uniform location for a specific uniform variable when we need to upload the uniforms for a draw command.
        std::unordered_map<std::string, size_t> uniformNameToIdCache; // A cache of uniform variable name to index mappings, this allows us to quickly access the index of a uniform variable by its name when we need to set a uniform variable from the user code.
        std::unordered_map<size_t, std::string> uniformIdToNameCache; // A cache of uniform variable index to name mappings, this allows us to quickly access the name of a uniform variable by its index when we need to upload the uniforms for a draw command, this is used to resolve the uniform location from the shader when we need to upload the uniforms for a draw command.
    };

    struct UniformCache
    {
        std::unordered_map<Shader*, ShaderUniformCache> shaderUniformCaches; // A map of shader pointers to their corresponding uniform caches, this allows us to quickly access the uniform cache for a specific shader when we need to upload the uniforms for a draw command.
    };

    ShaderUniformCache shader_uniform_cache_create();
    void shader_uniform_cache_insert_or_update(ShaderUniformCache& cache, const std::string& name, const UniformVariable& variable);
    int shader_uniform_cache_get_location(ShaderUniformCache& cache, size_t id, const std::function<int(const std::string&)>& locationResolver);
    UniformVariable& shader_uniform_cache_get_variable(ShaderUniformCache& cache, size_t id);
    void shader_uniform_cache_mark_upload(ShaderUniformCache& cache, const UniformIds& uniformIds);

    UniformCache uniform_cache_create();
    ShaderUniformCache& uniform_cache_get_shader_cache(UniformCache& cache, Shader* shader);
} // namespace p5
