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
    // regular draw calls (including image()) batch up to 8 textures into units 0-7 (see
    // u_Textures[8] in the default shaders). Pick unit >= 1 for extra samplers in a
    // custom loadEffectShader() effect (its own `tex` argument always uses unit 0), or
    // unit >= 8 for a custom loadShader().
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

    namespace detail
    {
        struct ShaderResource;
    }

    struct Shader;

    // Compiles and links a shader program, caching by (vertexShaderSource,
    // fragmentShaderSource) pair — a repeated call with the same source pair returns a
    // Shader that aliases the same underlying compiled program instead of recompiling.
    // Returns an invalid Shader (see isShaderValid()) on a compile/link error.
    Shader loadShader(std::string_view vertexShaderSource, std::string_view fragmentShaderSource);

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
    // A compiled+linked shader program handle. Copies are cheap and alias the same GL
    // program (shared_ptr-backed); the program is deleted automatically once the last
    // copy is destroyed. Default-constructed instances are "invalid" (id.value == 0) -
    // see isShaderValid().
    struct Shader
    {
        ShaderId id;

        // Internal handle driving automatic cleanup - not meant to be read or written
        // directly. Public (rather than private+friend) because detail::ShaderResource
        // is opaque outside shader.cpp: exposing the pointer can't be used to fabricate
        // a working Shader, only to alias or null out this one.
        std::shared_ptr<detail::ShaderResource> resource;
    };

    bool isShaderValid(const Shader& shader);
    std::optional<UniformLocation> getUniformLocation(const Shader& shader, const std::string& name);
} // namespace p5cpp

namespace p5cpp
{
    // Built-in convenience shaders for common full-screen effects - regular Shaders, so
    // apply them exactly like any other shader() call: draw whatever you want affected
    // while the shader is active (e.g. image() on a Framebuffer's color texture you
    // rendered into via pushCanvas()/popCanvas()), setting whatever uniforms it needs
    // first via setUniform(). There is no dedicated "effect" or "filter" API - this is
    // just where the GLSL for these particular effects happens to live.
    //
    // Uniforms:
    //   u_Amount (float, 0..1) - grayscale/invert mix amount, threshold cutoff.
    Shader loadGrayscaleShader();
    Shader loadInvertShader();
    Shader loadThresholdShader();

    // Separable gaussian blur - requires two passes, one per axis, each rendered into a
    // separate target (the second pass reads the first pass's output):
    //
    //     shader(blurShader);
    //     setUniform(blurShader, "u_TexelSize", uniform(1.0f / w, 1.0f / h));
    //     setUniform(blurShader, "u_Radius", uniform(radius));
    //     setUniform(blurShader, "u_Direction", uniform(1.0f, 0.0f));
    //     image(source.colorTexture, 0, 0, w, h);   // -> horizontal pass target
    //     setUniform(blurShader, "u_Direction", uniform(0.0f, 1.0f));
    //     image(horizontalPass.colorTexture, 0, 0, w, h); // -> final target
    //     noShader();
    //
    // Uniforms: u_TexelSize (vec2, 1/width 1/height), u_Direction (vec2, (1,0) or (0,1)),
    // u_Radius (float, blur radius in pixels).
    Shader loadBlurShader();
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
    inline UniformVariable uniform(const Texture& texture, int32_t unit) { return UniformVariable {.type = UniformVariable::Type::texture, .textureValue = {texture.id, unit}}; }
} // namespace p5cpp
