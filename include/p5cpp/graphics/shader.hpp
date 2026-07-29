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
#include <variant>

namespace p5cpp
{
    struct TextureUniformValue
    {
        TextureId textureId;
        int32_t unit;

        constexpr bool operator==(const TextureUniformValue& other) const = default;
    };

    using UniformVariable = std::variant<float, float2, float3, float4, matrix4x4, TextureUniformValue>;

    constexpr UniformVariable uniform(float x);
    constexpr UniformVariable uniform(float x, float y);
    constexpr UniformVariable uniform(float x, float y, float z);
    constexpr UniformVariable uniform(float x, float y, float z, float w);
    constexpr UniformVariable uniform(const matrix4x4& value);
    constexpr UniformVariable uniform(const Texture& texture, int32_t unit);
} // namespace p5cpp

namespace p5cpp
{
    struct UniformLocation
    {
        int32_t value;

        inline constexpr bool operator==(const UniformLocation& other) const = default;
    };

    struct ShaderId
    {
        uint32_t value;

        constexpr bool operator==(const ShaderId& other) const = default;
    };

    struct Shader
    {
        ShaderId id;
    };

    Shader loadShader(std::string_view vertexShaderSource, std::string_view fragmentShaderSource);
    bool isShaderValid(const Shader& shader);
    std::optional<UniformLocation> getUniformLocation(const Shader& shader, const std::string& name);

    // Compiles a custom full-screen post-processing shader from a small GLSL snippet, for
    // use with shader() like any other shader (see loadGrayscaleShader() etc. below for
    // the apply pattern). Unlike loadShader(), the caller does not need to write a vertex
    // shader or the multi-texture sampling boilerplate — `effectSource` only has to
    // define:
    //
    //     vec4 effect(vec2 uv, vec2 texelSize, sampler2D tex)
    //
    // `tex` is whatever texture the draw call while this shader is active samples (e.g.
    // the color texture of a Framebuffer you rendered into), `uv` is the texture
    // coordinate to sample/output for, and `texelSize` is a uniform (1/width, 1/height)
    // the caller is responsible for setting via setUniform() if the effect uses it.
    Shader loadEffectShader(std::string_view effectSource);
} // namespace p5cpp

namespace p5cpp
{
    // Built-in convenience shaders - apply like any other shader() call.
    // Uniforms: u_Amount (float, 0..1) - grayscale/invert mix amount, threshold cutoff.
    Shader loadGrayscaleShader();
    Shader loadInvertShader();
    Shader loadThresholdShader();

    // Separable gaussian blur - needs two passes, one per axis (see examples/custom_effects).
    // Uniforms: u_TexelSize (vec2, 1/width 1/height), u_Direction (vec2, (1,0) or (0,1)),
    // u_Radius (float, blur radius in pixels).
    Shader loadBlurShader();
} // namespace p5cpp

namespace p5cpp
{
    void unload(Shader& shader); // no-op if already unloaded/invalid
} // namespace p5cpp

namespace p5cpp
{
    inline constexpr UniformVariable uniform(float x) { return UniformVariable {x}; }
    inline constexpr UniformVariable uniform(float x, float y) { return UniformVariable {float2 {x, y}}; }
    inline constexpr UniformVariable uniform(float x, float y, float z) { return UniformVariable {float3 {x, y, z}}; }
    inline constexpr UniformVariable uniform(float x, float y, float z, float w) { return UniformVariable {float4 {x, y, z, w}}; }
    inline constexpr UniformVariable uniform(const matrix4x4& value) { return UniformVariable {value}; }
    inline constexpr UniformVariable uniform(const Texture& texture, int32_t unit) { return UniformVariable {TextureUniformValue {texture.id, unit}}; }
} // namespace p5cpp
