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

        constexpr bool operator==(const ShaderId& other) const = default;
        constexpr bool operator!=(const ShaderId& other) const = default;
    };

    struct ShaderImpl
    {
        virtual ~ShaderImpl() = default;

        virtual std::optional<UniformLocation> getUniformLocation(const std::string& name) const = 0;
        virtual ShaderId getShaderId() const = 0;
    };

    std::unique_ptr<ShaderImpl> loadShader(std::string_view vertexShaderSource, std::string_view fragmentShaderSource);
} // namespace p5cpp

namespace p5cpp
{
    class Shader : public ShaderImpl
    {
    public:
        Shader();
        Shader(std::shared_ptr<ShaderImpl> shader);

        std::optional<UniformLocation> getUniformLocation(const std::string& name) const override;
        virtual ShaderId getShaderId() const override;

    private:
        std::shared_ptr<ShaderImpl> shader;
    };

} // namespace p5cpp
