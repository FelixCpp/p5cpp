#pragma once

#include <p5cpp/p5cpp.hpp>

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

        std::unique_ptr<float2[]> m_positions;
        std::unique_ptr<float2[]> m_texCoords;
        std::unique_ptr<color_t[]> m_fillColors;
        std::unique_ptr<color_t[]> m_strokeColors;
        size_t m_vertexCount;
        size_t m_vertexCapacity;

        std::vector<float2> m_curvePoints; // raw points passed to curveVertex(), before Catmull-Rom subdivision
    };
} // namespace p5
