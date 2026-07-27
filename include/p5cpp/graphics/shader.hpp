#pragma once

#include <p5cpp/math/value2.hpp>
#include <p5cpp/math/value3.hpp>
#include <p5cpp/math/value4.hpp>
#include <p5cpp/math/matrix4x4.hpp>
#include <p5cpp/graphics/texture.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <optional>
#include <memory>

namespace p5cpp
{
    // A sampler2D uniform: the texture unit it should be bound to (see uniform(const
    // Texture&, int32_t)) plus the texture to bind there.
    struct TextureUniformValue
    {
        TextureId textureId;
        int32_t unit;

        constexpr bool operator==(const TextureUniformValue& other) const = default;
    };

    struct UniformVariable
    {
        enum class Type {
            float1,
            float2,
            float3,
            float4,
            matrix4x4,
            texture
        } type;

        union
        {
            float floatValue;
            float2 float2Value;
            float3 float3Value;
            float4 float4Value;
            matrix4x4 matrix4x4Value;
            TextureUniformValue textureValue;
        };
    };

    constexpr UniformVariable uniform(float x);
    constexpr UniformVariable uniform(float x, float y);
    constexpr UniformVariable uniform(float x, float y, float z, float w);
    constexpr UniformVariable uniform(const matrix4x4& value);

    // Binds `texture` to the given texture unit and assigns it to a sampler2D uniform.
    // `unit` must not collide with the texture units the renderer manages automatically:
    // regular draw calls batch up to 8 textures into units 0-7 (see u_Textures[8] in the
    // default shaders), and effect() passes always bind the source canvas to unit 0. Pick
    // unit >= 1 for extra samplers in a custom effect shader (loadEffectShader() effects
    // never use more than unit 0 internally), or unit >= 8 for a custom loadShader().
    UniformVariable uniform(const Texture& texture, int32_t unit);
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

    // Compiles a custom full-screen post-processing effect from a small GLSL snippet,
    // for use with effect(). Unlike loadShader(), the caller does not need to write a
    // vertex shader or the multi-texture sampling boilerplate — `effectSource` only has
    // to define:
    //
    //     vec4 effect(vec2 uv, vec2 texelSize, sampler2D tex)
    //
    // `tex` is the canvas contents before the effect runs, `uv` is the texture coordinate
    // to sample/output for, and `texelSize` is (1/width, 1/height) of the current canvas.
    std::unique_ptr<ShaderImpl> loadEffectShader(std::string_view effectSource);
} // namespace p5cpp

namespace p5cpp
{
    class Shader : public ShaderImpl
    {
    public:
        Shader();
        Shader(std::unique_ptr<ShaderImpl> shader);
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
    inline constexpr UniformVariable uniform(float x, float y, float z) { return UniformVariable {.type = UniformVariable::Type::float3, .float3Value = float3 {x, y, z}}; }
    inline constexpr UniformVariable uniform(float x, float y, float z, float w) { return UniformVariable {.type = UniformVariable::Type::float4, .float4Value = float4 {x, y, z, w}}; }
    inline constexpr UniformVariable uniform(const matrix4x4& value) { return UniformVariable {.type = UniformVariable::Type::matrix4x4, .matrix4x4Value = value}; }
    inline UniformVariable uniform(const Texture& texture, int32_t unit) { return UniformVariable {.type = UniformVariable::Type::texture, .textureValue = {texture.getTextureId(), unit}}; }
} // namespace p5cpp
