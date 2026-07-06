#pragma once

#include <p5cpp/math/value2.hpp>
#include <p5cpp/math/value4.hpp>
#include <p5cpp/math/matrix4x4.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <optional>
#include <memory>

namespace p5cpp
{
    struct UniformVariable
    {
        enum class Type {
            float1,
            float2,
            float4,
            matrix4x4
        } type;

        union
        {
            float floatValue;
            float2 float2Value;
            float4 float4Value;
            matrix4x4 matrix4x4Value;
        };
    };

    constexpr UniformVariable uniform(float x);
    constexpr UniformVariable uniform(float x, float y);
    constexpr UniformVariable uniform(float x, float y, float z, float w);
    constexpr UniformVariable uniform(const matrix4x4& value);
} // namespace p5cpp

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

    struct ShaderHasher
    {
        constexpr size_t operator()(const ShaderId& shaderId) const noexcept;
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

namespace p5cpp
{
    inline constexpr size_t ShaderHasher::operator()(const ShaderId& shaderId) const noexcept
    {
        return std::hash<uint32_t>()(shaderId.value);
    }
} // namespace p5cpp

namespace p5cpp
{
    inline constexpr UniformVariable uniform(float x) { return UniformVariable {.type = UniformVariable::Type::float1, .floatValue = x}; }
    inline constexpr UniformVariable uniform(float x, float y) { return UniformVariable {.type = UniformVariable::Type::float2, .float2Value = float2 {x, y}}; }
    inline constexpr UniformVariable uniform(float x, float y, float z, float w) { return UniformVariable {.type = UniformVariable::Type::float4, .float4Value = float4 {x, y, z, w}}; }
    inline constexpr UniformVariable uniform(const matrix4x4& value) { return UniformVariable {.type = UniformVariable::Type::matrix4x4, .matrix4x4Value = value}; }
} // namespace p5cpp
