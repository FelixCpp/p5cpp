#pragma once

#include "p5.hpp"

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <functional>

namespace p5
{
    using UniformIds = std::unordered_set<size_t>;

    struct UniformEntry
    {
        std::string name;
        UniformVariable variable;
        bool dirty;
    };

    UniformEntry uniform_entry_create(const std::string& name, const UniformVariable& variable);

    // This struct represents a a cache for a single shader.
    struct ShaderUniformCache
    {
        std::vector<UniformEntry> uniforms;                         // A list of uniform variables that have been set by the user.
        std::unordered_map<std::string, size_t> uniformNameToIndex; // A cache of uniform variable name to index mappings, this allows us to quickly access the index of a uniform variable by its name when we need to set a uniform variable from the user code.
        std::unordered_map<std::string, int> uniformNameToLocation;
    };

    struct UniformCache
    {
        std::unordered_map<Shader*, ShaderUniformCache> shaderUniformCaches; // A map of shader pointers to their corresponding uniform caches, this allows us to quickly access the uniform cache for a specific shader when we need to upload the uniforms for a draw command.
    };

    ShaderUniformCache shader_uniform_cache_create();
    void shader_uniform_cache_insert_or_update(ShaderUniformCache& cache, const std::string& name, const UniformVariable& variable);
    void shader_uniform_cache_mark_upload(ShaderUniformCache& cache);
    int shader_uniform_cache_get_location(ShaderUniformCache& cache, const std::string& name, const std::function<int(const std::string&)>& locationResolver);

    UniformCache uniform_cache_create();
    ShaderUniformCache& uniform_cache_get_shader_cache(UniformCache& cache, Shader* shader);
} // namespace p5
