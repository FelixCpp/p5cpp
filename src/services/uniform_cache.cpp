#include "uniform_cache.hpp"

namespace p5cpp
{
    void UniformCache::setUniform(Shader* shader, const std::string& name, const UniformVariable& variable)
    {
        const std::optional<UniformLocation> location = shader->getUniformLocation(name);
        if (not location.has_value()) {
            return;
        }

        const UniformSnapshot newSnapshot {
            .location = location.value(),
            .variable = variable,
        };

        const auto insertion = uniformsByShader.try_emplace(
            shader,
            std::vector<UniformSnapshot> {newSnapshot}
        );

        const bool hasBeenInserted = insertion.second;
        if (not hasBeenInserted) {
            std::vector<UniformSnapshot>& snapshots = insertion.first->second;
            const auto variableItr = std::find_if(snapshots.begin(), snapshots.end(), [location](const UniformSnapshot& snapshot) {
                return snapshot.location == location;
            });

            const bool variableExists = variableItr != snapshots.end();
            if (variableExists) {
                variableItr->variable = variable;
            } else {
                snapshots.push_back(newSnapshot);
            }
        }

        dirtyShaders.insert(shader);
    }

    void UniformCache::markShaderClean(Shader* shader)
    {
        dirtyShaders.erase(shader);
    }

    bool UniformCache::isShaderDirty(Shader* shader) const
    {
        return dirtyShaders.contains(shader);
    }

    std::vector<UniformSnapshot> UniformCache::getUniforms(Shader* shader)
    {
        const auto itr = uniformsByShader.find(shader);
        if (itr != uniformsByShader.end()) {
            return itr->second;
        }

        return {};
    }
} // namespace p5cpp
