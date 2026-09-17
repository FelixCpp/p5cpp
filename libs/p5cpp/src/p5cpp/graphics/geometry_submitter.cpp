#include <p5cpp/graphics/geometry_submitter.hpp>
#include <p5cpp/graphics/tessellators.hpp>

#include <algorithm>

namespace p5
{
    namespace
    {
        float4 toFloat4(color_t color)
        {
            return {
                static_cast<float>(color.r) / 255.0f,
                static_cast<float>(color.g) / 255.0f,
                static_cast<float>(color.b) / 255.0f,
                static_cast<float>(color.a) / 255.0f,
            };
        }
    } // namespace

    GeometrySubmitter::GeometrySubmitter(Renderer& renderer, Texture defaultTexture, Shader defaultFillShader, Shader defaultTextShader)
        : m_renderer(renderer),
          m_defaultTexture(std::move(defaultTexture)),
          m_defaultFillShader(std::move(defaultFillShader)),
          m_defaultTextShader(std::move(defaultTextShader))
    {
    }

    Shader GeometrySubmitter::resolveActiveShader(const DrawState& state, const Shader& fallback) const
    {
        if (state.shader.isValid()) {
            return state.shader;
        }

        return fallback;
    }

    Texture GeometrySubmitter::resolveActiveTexture(const Texture& texture) const
    {
        if (texture.isValid()) {
            return texture;
        }

        return m_defaultTexture;
    }

    void GeometrySubmitter::submitMesh(const Renderer::Writer& writer, const DrawState& state, const Texture& texture, const Shader& shader, TextureFilter filter, TextureWrap wrap)
    {
        m_renderer.finish(writer, state.blendMode, state.clipRect, filter, wrap, texture, shader, state.shaderUniforms);
    }

    void GeometrySubmitter::submitQuad(const std::span<const float2, 4>& positions, const std::span<const float2, 4>& texCoords, color_t color, const DrawState& state, const Texture& texture)
    {
        const float4 col = toFloat4(color);
        const float4 colors[4] = {col, col, col, col};

        Renderer::Writer writer = m_renderer.write();
        tesselate_quad(writer, positions, texCoords, colors);
        submitMesh(writer, state, resolveActiveTexture(texture), resolveActiveShader(state, m_defaultFillShader), state.textureFilter, state.textureWrap);
    }

    void GeometrySubmitter::submitStroke(const std::span<const float2>& positions, bool closed, color_t color, const DrawState& state)
    {
        std::vector<float2> texCoords(positions.size());
        float pathLength = 0.0f;
        for (size_t i = 0; i < positions.size(); ++i) {
            if (i > 0)
                pathLength += distance(positions[i - 1], positions[i]);
            texCoords[i] = {pathLength, 0.0f};
        }

        const std::vector<color_t> colors(positions.size(), color);
        submitStroke(positions, texCoords, colors, closed, state, /*synthesizeCrossTrackV=*/true);
    }

    void GeometrySubmitter::submitStroke(const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const color_t>& colors, bool closed, const DrawState& state, bool synthesizeCrossTrackV)
    {
        std::vector<float4> convertedColors(colors.size());
        std::ranges::transform(colors, convertedColors.begin(), toFloat4);

        Renderer::Writer writer = m_renderer.write();
        tesselate_path(writer, positions, texCoords, convertedColors, state.strokeWeight, state.strokeCap, state.strokeJoin, state.strokeMiterLimit, state.strokeRoundJoinThreshold, closed, synthesizeCrossTrackV);
        submitMesh(writer, state, resolveActiveTexture(), resolveActiveShader(state, m_defaultFillShader), state.textureFilter, state.textureWrap);
    }

    void GeometrySubmitter::submitFillMesh(ShapeMode mode, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const color_t>& colors, const DrawState& state)
    {
        std::vector<float4> convertedColors(colors.size());
        std::ranges::transform(colors, convertedColors.begin(), toFloat4);

        Renderer::Writer writer = m_renderer.write();
        switch (mode) {
            case ShapeMode::triangles: tesselate_triangles(writer, positions, texCoords, convertedColors); break;
            case ShapeMode::triangleStrip: tesselate_triangle_strip(writer, positions, texCoords, convertedColors); break;
            case ShapeMode::triangleFan: tesselate_triangle_fan(writer, positions, texCoords, convertedColors); break;
            case ShapeMode::quads: tesselate_quads(writer, positions, texCoords, convertedColors); break;
            case ShapeMode::quadStrip: tesselate_quad_strip(writer, positions, texCoords, convertedColors); break;
            case ShapeMode::points:
            case ShapeMode::lines:
            case ShapeMode::path: return;
            case ShapeMode::polygon:
            default: tesselate_polygon(writer, positions, texCoords, convertedColors); break;
        }

        submitMesh(writer, state, resolveActiveTexture(state.texture), resolveActiveShader(state, m_defaultFillShader), state.textureFilter, state.textureWrap);
    }

    void GeometrySubmitter::submitTextMesh(const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const color_t>& colors, const Texture& atlasTexture, const DrawState& state)
    {
        std::vector<float4> convertedColors(colors.size());
        std::ranges::transform(colors, convertedColors.begin(), toFloat4);

        Renderer::Writer writer = m_renderer.write();
        tesselate_quads(writer, positions, texCoords, convertedColors);
        // Deliberately not state.textureFilter/textureWrap -- confirmed to keep as-is (glyph
        // atlas sampling has no reason to want anything but linear/clampToEdge in practice).
        submitMesh(writer, state, atlasTexture, resolveActiveShader(state, m_defaultTextShader), TextureFilter::linear, TextureWrap::clampToEdge);
    }
} // namespace p5
