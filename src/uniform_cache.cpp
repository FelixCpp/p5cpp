#include "uniform_cache.hpp"

namespace p5
{
    ShaderUniformCache shader_uniform_cache_create()
    {
        return ShaderUniformCache {
            .uniforms = {},
            .dirtyUniformIndices = {},
            .uniformLocationCache = {},
            .uniformNameToIdCache = {},
            .uniformIdToNameCache = {},
        };
    }

    void shader_uniform_cache_insert_or_update(ShaderUniformCache& cache, const std::string& name, const UniformVariable& variable)
    {
        const auto itr = cache.uniformNameToIdCache.find(name);
        if (itr == cache.uniformNameToIdCache.end()) {
            const size_t id = cache.uniforms.size();
            cache.uniforms.push_back(variable);
            cache.uniformNameToIdCache.emplace(name, id);
            cache.uniformIdToNameCache.emplace(id, name);
            cache.dirtyUniformIndices.insert(id);
            return;
        }

        const size_t id = itr->second;
        cache.uniforms[id] = variable;
        cache.dirtyUniformIndices.insert(id);
    }

    int shader_uniform_cache_get_location(ShaderUniformCache& cache, size_t id, const std::function<int(const std::string&)>& locationResolver)
    {
        const auto itr = cache.uniformLocationCache.find(id);
        if (itr != cache.uniformLocationCache.end()) {
            return itr->second;
        }

        const std::string& name = cache.uniformIdToNameCache.at(id);
        const int location = locationResolver(name);
        cache.uniformLocationCache.emplace(id, location);
        return location;
    }

    UniformVariable& shader_uniform_cache_get_variable(ShaderUniformCache& cache, size_t id)
    {
        return cache.uniforms[id];
    }

    void shader_uniform_cache_mark_upload(ShaderUniformCache& cache, const UniformIds& uniformIds)
    {
        cache.dirtyUniformIndices.clear();
        // for (const size_t id : uniformIds) {
        //     cache.dirtyUniformIndices.erase(id);
        // }
    }
} // namespace p5

namespace p5
{
    UniformCache uniform_cache_create()
    {
        return UniformCache {
            .shaderUniformCaches = {},
        };
    }

    ShaderUniformCache& uniform_cache_get_shader_cache(UniformCache& cache, Shader* shader)
    {
        return cache.shaderUniformCaches.try_emplace(shader, shader_uniform_cache_create()).first->second;
    }
} // namespace p5
