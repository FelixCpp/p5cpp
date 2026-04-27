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

        void stroke(DrawScope& scope, const DrawPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, StrokeAlign strokeAlign, float miterLimit, bool close) override
        {
            const int n = points.size;
            if (n < 2) return;
            float outerOffset, innerOffset;
            switch (strokeAlign) {
                case StrokeAlign::inside:
                    outerOffset = 0.0f;
                    innerOffset = strokeWeight;
                    break;
                case StrokeAlign::center:
                    outerOffset = strokeWeight * 0.5f;
                    innerOffset = strokeWeight * 0.5f;
                    break;
                case StrokeAlign::outside:
                    outerOffset = strokeWeight;
                    innerOffset = 0.0f;
                    break;
            }

            for (size_t i = 0; i < n - 1; ++i) {
                float2 p0 = points.positions[i];
                float2 p1 = points.positions[i + 1];

                color_t c0 = points.colors[i];
                color_t c1 = points.colors[i + 1];

                float2 dir = normalized(p1 - p0);
                float2 normal = perp(dir);

                float2 v0o = p0 + normal * outerOffset;
                float2 v0i = p0 - normal * innerOffset;
                float2 v1o = p1 + normal * outerOffset;
                float2 v1i = p1 - normal * innerOffset;

                uint32_t base = scope.getVertexCount();
                scope.push(Vertex {.position = v0o, .texcoord = {0.0f, 0.0f}, .color = color_to_float4(c0)});
                scope.push(Vertex {.position = v0i, .texcoord = {0.0f, 0.0f}, .color = color_to_float4(c0)});
                scope.push(Vertex {.position = v1o, .texcoord = {0.0f, 0.0f}, .color = color_to_float4(c1)});
                scope.push(Vertex {.position = v1i, .texcoord = {0.0f, 0.0f}, .color = color_to_float4(c1)});
                pushQuad(scope, base + 0, base + 1, base + 2, base + 3);
            }

            // ── Caps ─────────────────────────────────────────────────────────────────
            {
                // Start-Cap
                float2 dir = normalized(points.positions[1] - points.positions[0]);
                float2 normal = perp(dir);
                float2 left = points.positions[0] + normal * outerOffset;
                float2 right = points.positions[0] - normal * innerOffset;
                buildCap(scope, points.positions[0], -dir, left, right, color_to_float4(points.colors[0]), strokeCap);
            }
            {
                // End-Cap
                float2 dir = normalized(points.positions[n - 1] - points.positions[n - 2]);
                float2 normal = perp(dir);
                float2 left = points.positions[n - 1] + normal * outerOffset;
                float2 right = points.positions[n - 1] - normal * innerOffset;
                buildCap(scope, points.positions[n - 1], dir, left, right, color_to_float4(points.colors[n - 1]), strokeCap);
            }
        }

    private:
        void buildCap(DrawScope& scope, float2 tip, float2 dir, float2 left, float2 right, float4 color, StrokeCap strokeCap)
        {
            switch (strokeCap) {
                case StrokeCap::butt:
                    // Nichts zu tun – das Quad des Segments schliesst bereits bündig ab
                    break;

                case StrokeCap::square: {
                    // Verlängerung um halfWidth in Richtung dir
                    float halfW = length(right - left) * 0.5f; // abgeleitet aus dem Segment
                    float2 extLeft = left + dir * halfW;
                    float2 extRight = right + dir * halfW;

                    uint32_t base = (uint32_t)scope.getVertexCount();
                    scope.push(Vertex {.position = left, .texcoord = {0, 0}, .color = color});     // 0
                    scope.push(Vertex {.position = right, .texcoord = {0, 0}, .color = color});    // 1
                    scope.push(Vertex {.position = extLeft, .texcoord = {0, 0}, .color = color});  // 2
                    scope.push(Vertex {.position = extRight, .texcoord = {0, 0}, .color = color}); // 3
                    pushQuad(scope, base + 0, base + 1, base + 2, base + 3);
                    break;
                }

                case StrokeCap::round: {
                    const int segments = 8;
                    float halfW = length(right - left) * 0.5f;
                    float2 center = tip;

                    float angleStart = std::atan2(right.y - center.y, right.x - center.x);
                    float angleEnd = std::atan2(left.y - center.y, left.x - center.x);

                    // Sicherstellen dass wir den richtigen Bogen nehmen (in dir-Richtung)
                    float diff = angleEnd - angleStart;
                    if (diff > (float)M_PI) diff -= 2.0f * (float)M_PI;
                    if (diff < -(float)M_PI) diff += 2.0f * (float)M_PI;

                    float step = diff / segments;

                    uint32_t centerIdx = scope.getVertexCount();
                    scope.push(Vertex {.position = center, .texcoord = {0, 0}, .color = color});

                    // Rim-Vertices vorab alle pushen
                    uint32_t firstRim = scope.getVertexCount();
                    for (int s = 0; s <= segments; ++s) {
                        float a = angleStart + step * s;
                        float2 pos = {center.x + std::cos(a) * halfW, center.y + std::sin(a) * halfW};
                        scope.push(Vertex {.position = pos, .texcoord = {0, 0}, .color = color});
                    }

                    // Dann alle Dreiecke
                    for (int s = 0; s < segments; ++s) {
                        scope.push(centerIdx);
                        scope.push(firstRim + s);
                        scope.push(firstRim + s + 1);
                    }
                    break;
                }
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
    };
} // namespace p5

namespace p5
{
    std::unique_ptr<Tesselator> createTesselator()
    {
        return std::make_unique<LibTessTesselator>();
    }
} // namespace p5
