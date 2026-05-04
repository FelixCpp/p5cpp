#include "tess.hpp"
#include <tesselator.h>

namespace p5
{
    inline static float4 color_to_float4(color_t color)
    {
        return float4 {
            .x = static_cast<float>(red(color)) / 255.0f,
            .y = static_cast<float>(green(color)) / 255.0f,
            .z = static_cast<float>(blue(color)) / 255.0f,
            .w = static_cast<float>(alpha(color)) / 255.0f,
        };
    }
} // namespace p5

namespace p5
{
    class ConvexTesselator : public Tesselator
    {
        void tesselate(DrawScope& scope, const DrawPoints& points) override
        {
            for (size_t i = 0; i < points.size; ++i) {
                scope.push(Vertex {
                    .position = points.positions[i],
                    .texcoord = points.texcoords[i],
                    .color = color_to_float4(points.colors[i]),
                });
            }

            for (size_t i = 2; i < points.size; ++i) {
                scope.push(0);
                scope.push(i - 1);
                scope.push(i);
            }
        }
    };

    class LibTessTesselator : public Tesselator
    {
    public:
        LibTessTesselator()
            : m_tess(tessNewTess(nullptr))
        {
        }

        ~LibTessTesselator() override
        {
            tessDeleteTess(m_tess);
        }

        void tesselate(DrawScope& scope, const DrawPoints& points) override
        {
            if (m_tessPoints.size() < points.size * 3) {
                m_tessPoints.resize(points.size * 3);
            }

            for (size_t i = 0; i < points.size; ++i) {
                m_tessPoints[i * 3 + 0] = points.positions[i].x;
                m_tessPoints[i * 3 + 1] = points.positions[i].y;
                m_tessPoints[i * 3 + 2] = 0.0f;
            }

            tessAddContour(m_tess, 3, m_tessPoints.data(), sizeof(float) * 3, points.size);
            if (not tessTesselate(m_tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 3, nullptr)) {
                return;
            }

            const TESSreal* tessVerts = tessGetVertices(m_tess);
            const TESSindex* tessIdx = tessGetElements(m_tess);
            const TESSindex* vertexIndex = tessGetVertexIndices(m_tess);
            const int vertexCount = tessGetVertexCount(m_tess);
            const int elemCount = tessGetElementCount(m_tess);

            for (int i = 0; i < vertexCount; ++i) {
                const int srcIdx = vertexIndex[i];

                if (srcIdx == TESS_UNDEF) {
                    scope.push(Vertex {
                        .position = float2 {tessVerts[i * 3 + 0], tessVerts[i * 3 + 1]},
                        .texcoord = float2 {0.0f, 0.0f},
                        .color = color_to_float4(points.colors[0]),
                    });
                } else {
                    scope.push(Vertex {
                        .position = points.positions[srcIdx],
                        .texcoord = points.texcoords[srcIdx],
                        .color = color_to_float4(points.colors[srcIdx]),
                    });
                }
            }

            int validIndexCount = 0;
            for (int i = 0; i < elemCount; ++i) {
                const int a = tessIdx[i * 3 + 0];
                const int b = tessIdx[i * 3 + 1];
                const int c = tessIdx[i * 3 + 2];
                if (a == TESS_UNDEF || b == TESS_UNDEF || c == TESS_UNDEF) continue;

                scope.push(a);
                scope.push(b);
                scope.push(c);
            }
        }

    private:
        TESStesselator* m_tess;
        std::vector<float> m_tessPoints;
    }; // namespace p5
} // namespace p5

namespace p5
{
    std::unique_ptr<Tesselator> createTesselator()
    {
        return std::make_unique<ConvexTesselator>();
        // return std::make_unique<LibTessTesselator>();
    }
} // namespace p5
