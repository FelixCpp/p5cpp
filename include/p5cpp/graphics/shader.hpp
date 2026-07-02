#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <optional>
#include <memory>

namespace p5cpp
{
    struct UniformLocation
    {
        int32_t value;

        inline constexpr bool operator==(const UniformLocation& other) const = default;
        inline constexpr bool operator!=(const UniformLocation& other) const = default;
    };

    struct ShaderId
    {
        uint32_t value;
    };

    struct Shader
    {
        virtual ~Shader() = default;

        virtual std::optional<UniformLocation> getUniformLocation(const std::string& name) const = 0;
        virtual ShaderId getShaderId() const = 0;
    };

    class ShaderHandle : public Shader
    {
    public:
        ShaderHandle();
        ShaderHandle(std::unique_ptr<Shader> shader);

        std::optional<UniformLocation> getUniformLocation(const std::string& name) const override;
        virtual ShaderId getShaderId() const override;

    private:
        std::unique_ptr<Shader> shader;
    };

    std::unique_ptr<Shader> loadShader(std::string_view vertexShaderSource, std::string_view fragmentShaderSource);
} // namespace p5cpp
