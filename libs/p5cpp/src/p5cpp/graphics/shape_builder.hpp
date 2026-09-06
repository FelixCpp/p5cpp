#pragma once

#include <p5cpp/p5cpp.hpp>

#include <vector>

namespace p5
{
    struct BuiltShape
    {
        ShapeMode mode;
        size_t vertexCount;
        std::span<const float2> positions;
        std::span<const float2> texCoords;
        std::span<const color_t> fillColors;
        std::span<const color_t> strokeColors;
    };

    class ShapeBuilder
    {
    public:
        explicit ShapeBuilder();

        void beginShape(ShapeMode mode);
        BuiltShape endShape();

        void vertex(float x, float y, color_t fillColor, color_t strokeColor);
        void vertex(float x, float y, float u, float v, color_t fillColor, color_t strokeColor);
        void bezierVertex(float controlX1, float controlY1, float controlX2, float controlY2, float endX, float endY);
        void quadraticVertex(float controlX, float controlY, float endX, float endY);
        void curveVertex(float x, float y, float tightness, color_t fillColor, color_t strokeColor);

    private:
        bool m_isBuilding;
        ShapeMode m_mode;

        std::vector<float2> m_positions;
        std::vector<float2> m_texCoords;
        std::vector<color_t> m_fillColors;
        std::vector<color_t> m_strokeColors;

        std::vector<float2> m_curvePoints;
    };
} // namespace p5
