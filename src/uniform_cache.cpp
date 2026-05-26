#include "uniform_cache.hpp"

namespace p5
{
    ShaderUniformCache shader_uniform_cache_create()
    {
        return ShaderUniformCache {
            .uniforms = {},
            .uniformNameToIndex = {},
        };
    }

    void shader_uniform_cache_insert_or_update(ShaderUniformCache& cache, const std::string& name, const UniformVariable& variable)
    {
        const auto itr = cache.uniformNameToIndex.find(name);
        if (itr == cache.uniformNameToIndex.end()) {
            const size_t id = cache.uniforms.size();

            UniformEntry entry = UniformEntry {
                .name = name,
                .variable = variable,
                .dirty = true,
            };

            cache.uniforms.emplace_back(std::move(entry));
            cache.uniformNameToIndex.emplace(name, id);
            return;
        }

        UniformEntry& entry = cache.uniforms[itr->second];
        entry.variable = variable;
        entry.dirty = true;
    }

    void shader_uniform_cache_mark_upload(ShaderUniformCache& cache)
    {
        for (UniformEntry& entry : cache.uniforms) {
            entry.dirty = false;
        }
    }

    int shader_uniform_cache_get_location(ShaderUniformCache& cache, const std::string& name, const std::function<int(const std::string&)>& locationResolver)
    {
        const auto itr = cache.uniformNameToLocation.find(name);
        if (itr != cache.uniformNameToLocation.end()) {
            return itr->second;
        }

        const int location = locationResolver(name);
        cache.uniformNameToLocation.emplace(name, location);
        return location;
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
