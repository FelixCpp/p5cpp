#include <p5cpp/graphics/uniform_cache.hpp>

namespace p5cpp
{
    void UniformCache::setUniform(const Shader& shader, const std::string& name, const UniformVariable& variable)
    {
        const std::optional<UniformLocation> location = getUniformLocation(shader, name);
        if (not location.has_value()) {
            return;
        }

        const UniformSnapshot newSnapshot {
            .location = location.value(),
            .variable = variable,
        };

        const auto insertion = uniformsByShader.try_emplace(
            shader.id,
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

        dirtyShaders.insert(shader.id);
    }

    void UniformCache::markShaderClean(const Shader& shader)
    {
        dirtyShaders.erase(shader.id);
    }

    bool UniformCache::isShaderDirty(const Shader& shader) const
    {
        return dirtyShaders.contains(shader.id);
    }

    std::vector<UniformSnapshot> UniformCache::getUniforms(const Shader& shader)
    {
        const auto itr = uniformsByShader.find(shader.id);
        if (itr != uniformsByShader.end()) {
            return itr->second;
        }

        return {};
    }
} // namespace p5cpp
