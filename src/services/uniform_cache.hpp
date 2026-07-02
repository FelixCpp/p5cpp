#pragma once

#include <p5cpp/p5cpp.hpp>

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
        void setUniform(Shader* shader, const std::string& name, const UniformVariable& variable);
        void markShaderClean(Shader* shader);
        bool isShaderDirty(Shader* shader) const;

        std::vector<UniformSnapshot> getUniforms(Shader* shader);

        std::unordered_map<Shader*, std::vector<UniformSnapshot>> uniformsByShader;
        std::unordered_set<Shader*> dirtyShaders;
    };
} // namespace p5cpp
