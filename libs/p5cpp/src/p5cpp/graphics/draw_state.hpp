#pragma once

#include <p5cpp/p5cpp.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace p5
{
    // Small, linear-scan association list for a shader's "extra uniforms". Uniform counts per
    // draw state are always small (well under 10 in practice), so this avoids paying an
    // unordered_map's allocation cost on every push()/pushState() regardless of whether uniforms
    // are even in use.
    using ShaderUniformList = std::vector<std::pair<std::string, UniformValue>>;

    struct DrawState
    {
        bool isFillEnabled = true;
        bool isStrokeEnabled = true;

        std::optional<rect2f> clipRect = std::nullopt;

        color_t fillColor = rgba(255, 255, 255);
        color_t strokeColor = rgba(255, 255, 255);
        color_t tintColor = rgba(255, 255, 255);
        float strokeWeight = 1.0f;

        StrokeCap strokeCap = StrokeCap::round;
        StrokeJoin strokeJoin = StrokeJoin::miter;
        float strokeMiterLimit = 10.0f;
        float strokeRoundJoinThreshold = radians(10.0f);
        float curveTightness = 0.0f;

        BlendMode blendMode = BlendMode::alpha;
        TextureUVMode textureUVMode = TextureUVMode::normalized;
        TextureFilter textureFilter = TextureFilter::linear;
        TextureWrap textureWrap = TextureWrap::clampToEdge;

        Texture texture;
        Shader shader;
        ShaderUniformList shaderUniforms;

        Font textFont;
        float textSize = 12.0f;
        TextAlignment textAlignment = TextAlignment::topLeft;
        TextWrap textWrap = TextWrap::none;
        std::optional<float> textLeadingOverride = std::nullopt;
        float textLetterSpacing = 0.0f;
        bool textLigatures = true;

        void setUniform(std::string_view name, UniformValue value)
        {
            for (auto& [existingName, existingValue] : shaderUniforms) {
                if (existingName == name) {
                    existingValue = std::move(value);
                    return;
                }
            }
            shaderUniforms.emplace_back(std::string(name), std::move(value));
        }
    };
} // namespace p5
