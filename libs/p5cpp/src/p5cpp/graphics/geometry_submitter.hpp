#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/draw_state.hpp>
#include <p5cpp/graphics/renderer.hpp>

#include <span>

namespace p5
{
    class GeometrySubmitter
    {
    public:
        GeometrySubmitter(Renderer& renderer, Texture defaultTexture, Shader defaultFillShader, Shader defaultTextShader);

        void submitQuad(const std::span<const float2, 4>& positions, const std::span<const float2, 4>& texCoords, color_t color, const DrawState& state, const Texture& texture = {});
        void submitStroke(const std::span<const float2>& positions, bool closed, color_t color, const DrawState& state);
        void submitStroke(const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const color_t>& colors, bool closed, const DrawState& state, bool synthesizeCrossTrackV = false);
        void submitFillMesh(ShapeMode mode, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const color_t>& colors, const DrawState& state);
        void submitTextMesh(const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const color_t>& colors, const Texture& atlasTexture, const DrawState& state);

    private:
        Shader resolveActiveShader(const DrawState& state, const Shader& fallback) const;
        Texture resolveActiveTexture(const Texture& texture = {}) const;

        void submitMesh(const Renderer::Writer& writer, const DrawState& state, const Texture& texture, const Shader& shader, TextureFilter filter, TextureWrap wrap);

        Renderer& m_renderer;
        Texture m_defaultTexture;
        Shader m_defaultFillShader;
        Shader m_defaultTextShader;
    };
} // namespace p5
