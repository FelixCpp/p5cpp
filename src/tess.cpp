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

        void fill(DrawScope& scope, const DrawPoints& points) override
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

        struct StrokeOffset
        {
            float inside;
            float outside;
        };

        StrokeOffset computeStrokeOffset(float strokeWeight, StrokeAlign strokeAlign)
        {
            switch (strokeAlign) {
                case StrokeAlign::inside:
                    return StrokeOffset {.inside = 0.0f, .outside = strokeWeight};
                case StrokeAlign::center:
                    return StrokeOffset {.inside = strokeWeight * 0.5f, .outside = strokeWeight * 0.5f};
                case StrokeAlign::outside:
                    return StrokeOffset {.inside = strokeWeight, .outside = 0.0f};
            }

            throw std::runtime_error("invalid stroke align");
        }

        void stroke(DrawScope& scope, const DrawPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, StrokeAlign strokeAlign, float miterLimit, bool close) override
        {
            float4 cols[] = {
                {1.0f, 0.0f, 0.0f, 1.0f},
                {0.0f, 1.0f, 0.0f, 1.0f},
                {0.0f, 0.0f, 1.0f, 1.0f},
                {0.0f, 1.0f, 1.0f, 1.0f},
            };

            const size_t n = points.size;
            if (n < 2) return;

            const auto [innerOffset, outerOffset] = computeStrokeOffset(strokeWeight, strokeAlign);

            for (size_t i = 0; i < n - 1; ++i) {
                float2 currPos = points.positions[i];
                float2 nextPos = points.positions[i + 1];

                color_t currCol = points.colors[i];
                color_t nextCol = points.colors[i + 1];

                float2 dir = normalized(nextPos - currPos);
                float2 normal = perp(dir);

                float2 v0o = currPos + normal * outerOffset;
                float2 v0i = currPos - normal * innerOffset;
                float2 v1o = nextPos + normal * outerOffset;
                float2 v1i = nextPos - normal * innerOffset;

                uint32_t base = scope.getVertexCount();
                scope.push(Vertex {.position = v0o, .texcoord = {0.0f, 0.0f}, .color = cols[(i + 0) % 4]});
                scope.push(Vertex {.position = v0i, .texcoord = {0.0f, 0.0f}, .color = cols[(i + 1) % 4]});
                scope.push(Vertex {.position = v1o, .texcoord = {0.0f, 0.0f}, .color = cols[(i + 2) % 4]});
                scope.push(Vertex {.position = v1i, .texcoord = {0.0f, 0.0f}, .color = cols[(i + 3) % 4]});
                pushQuad(scope, base + 0, base + 1, base + 2, base + 3);
            }
        }

    private:
        // a--c
        // |\ |
        // | \|
        // b--d
        void pushQuad(DrawScope& scope, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
        {
            scope.push(a);
            scope.push(b);
            scope.push(c);
            scope.push(b);
            scope.push(d);
            scope.push(c);
        }

        TESStesselator* m_tess;
        std::vector<float> m_tessPoints;
    }; // namespace p5
} // namespace p5

namespace p5
{
    std::unique_ptr<Tesselator> createTesselator()
    {
        return std::make_unique<LibTessTesselator>();
    }
} // namespace p5
