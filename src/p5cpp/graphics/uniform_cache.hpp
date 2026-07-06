#pragma once

#include <p5cpp/graphics/shader.hpp>

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace p5cpp
{
    struct UniformSnapshot
    {
        UniformLocation location;
        UniformVariable variable;
    };

    class UniformCache
    {
    public:
        void setUniform(const Shader& shader, const std::string& name, const UniformVariable& variable);
        void markShaderClean(const Shader& shader);
        bool isShaderDirty(const Shader& shader) const;

        std::vector<UniformSnapshot> getUniforms(const Shader& shader);

        std::unordered_map<ShaderId, std::vector<UniformSnapshot>> uniformsByShader;
        std::unordered_set<ShaderId> dirtyShaders;
    };
} // namespace p5cpp
