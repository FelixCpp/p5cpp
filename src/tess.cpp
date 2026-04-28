#include "tess.hpp"
#include <numbers>
#include <tesselator.h>
#include <algorithm>

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

    struct StrokeVertex
    {
        uint32_t index;
        float2 position;
        color_t color;
    };

    struct StrokeEdge
    {
        StrokeVertex inside;
        StrokeVertex outside;
    };

    class StrokeScope
    {
    public:
        explicit StrokeScope(DrawScope& scope) : scope(scope) {}

        StrokeVertex emit(float2 position, color_t color)
        {
            uint32_t idx = static_cast<uint32_t>(scope.getVertexCount());
            scope.push(Vertex {.position = position, .texcoord = {0.0f, 0.0f}, .color = color_to_float4(color)});
            return {idx, position, color};
        }

        // Gibt ein Quad aus zwei Edges aus (= ein Segment-Rechteck)
        void emitQuad(const StrokeEdge& a, const StrokeEdge& b)
        {
            //  a.outside --- b.outside
            //     |         /   |
            //  a.inside --- b.inside
            scope.push(a.outside.index);
            scope.push(a.inside.index);
            scope.push(b.outside.index);
            scope.push(a.inside.index);
            scope.push(b.inside.index);
            scope.push(b.outside.index);
        }

        // Gibt ein einzelnes Dreieck aus
        void emitTriangle(const StrokeVertex& a, const StrokeVertex& b, const StrokeVertex& c)
        {
            scope.push(a.index);
            scope.push(b.index);
            scope.push(c.index);
        }

    private:
        DrawScope& scope;
    };

    static StrokeEdge makeEdge(StrokeScope& ss, float2 point, float2 normal, float halfStroke, color_t color)
    {
        return {
            .inside = ss.emit(point - normal * halfStroke, color),
            .outside = ss.emit(point + normal * halfStroke, color),
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

        void stroke(DrawScope& drawScope, const DrawPoints& points, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, StrokeAlign strokeAlign, float miterLimit, bool close) override
        {
            if (points.size < 2) return;

            StrokeScope ss(drawScope);
            const float halfStroke = strokeWeight * 0.5f;
            const size_t n = points.size;
            const size_t segmentCount = close ? n : n - 1;

            auto edgeNormal = [&points](size_t iA, size_t iB) -> float2 {
                return perp(normalized(points.positions[iB] - points.positions[iA]));
            };

            StrokeEdge prevEdge;
            StrokeEdge firstEdge; // für close=true

            for (size_t i = 0; i < segmentCount; i++) {
                const size_t iA = i;
                const size_t iB = (i + 1) % n;

                const float2 posA = points.positions[iA];
                const float2 posB = points.positions[iB];
                const color_t colA = points.colors[iA];
                const color_t colB = points.colors[iB];
                const float2 norm = edgeNormal(iA, iB);

                const StrokeEdge edgeA = makeEdge(ss, posA, norm, halfStroke, colA);
                const StrokeEdge edgeB = makeEdge(ss, posB, norm, halfStroke, colB);

                if (i == 0) {
                    // if (!close)
                    //     emitCap(ss, posA, -norm, halfStroke, strokeCap, colA, true);
                    firstEdge = edgeA;
                } else {
                    const float2 prevNorm = edgeNormal(i - 1, iA);
                    emitJoin(ss, posA, prevNorm, norm, halfStroke, strokeJoin, miterLimit, colA, prevEdge, edgeA);
                }

                // ss.emitQuad(edgeA, edgeB);
                prevEdge = edgeB;

                if (i == segmentCount - 1) {
                    if (close)
                        emitJoin(ss, posB, norm, edgeNormal(0, 1), halfStroke, strokeJoin, miterLimit, colB, prevEdge, firstEdge);
                    // else
                    //     emitCap(ss, posB, norm, halfStroke, strokeCap, colB, false);
                }
            }
        }

        static void emitJoin(StrokeScope& ss, float2 point, float2 prevNorm, float2 nextNorm, float halfStroke, StrokeJoin join, float miterLimit, color_t color,
                             const StrokeEdge& prevEdge, // Ende des vorherigen Segments
                             const StrokeEdge& nextEdge) // Anfang des nächsten Segments
        {
            const float turn = cross(prevNorm, nextNorm);
            if (std::abs(turn) < 1e-6f) return;

            const bool gapOnPerpSide = turn > 0.0f;

            const StrokeVertex& outsidePrev = gapOnPerpSide ? prevEdge.outside : prevEdge.inside;
            const StrokeVertex& outsideNext = gapOnPerpSide ? nextEdge.outside : nextEdge.inside;
            const StrokeVertex& insidePrev = gapOnPerpSide ? prevEdge.inside : prevEdge.outside;
            const StrokeVertex& insideNext = gapOnPerpSide ? nextEdge.inside : nextEdge.outside;

            const float2 miterVec = normalized(prevNorm + nextNorm);
            const float miterLen = halfStroke / dot(miterVec, prevNorm);

            if (join == StrokeJoin::miter) {
                // Miter-Limit: wenn der Miter zu spitz wird → Fallback auf Bevel
                if (miterLen / halfStroke <= miterLimit) {
                    const StrokeVertex miterVertex = ss.emit(point + miterVec * miterLen, color);
                    // Dreieck: inside, außen-prev, miter-punkt, außen-next
                    ss.emitTriangle(insidePrev, outsidePrev, miterVertex);
                    ss.emitTriangle(insidePrev, miterVertex, outsideNext);
                    ss.emitTriangle(insidePrev, insideNext, outsideNext);
                } else {
                    // Bevel-Fallback
                    ss.emitTriangle(insidePrev, outsidePrev, outsideNext);
                    ss.emitTriangle(insidePrev, insideNext, outsideNext);
                }
            } else if (join == StrokeJoin::bevel) {
                ss.emitTriangle(insidePrev, outsidePrev, outsideNext);
                ss.emitTriangle(insidePrev, insideNext, outsideNext);
            } else if (join == StrokeJoin::round) {
                // Kreisbogen zwischen outsidePrev und outsideNext
                // Anzahl Segmente abhängig vom Winkel und strokeWeight
                const float angle = std::acos(std::clamp(dot(prevNorm, nextNorm), -1.f, 1.f));
                const int steps = std::max(1, static_cast<int>(angle / (2.f * std::numbers::pi_v<float>)*32));
                const float stepAngle = angle / static_cast<float>(steps);

                // Startwinkel: Richtung von outsidePrev vom Punkt aus
                const float startAngle = std::atan2(prevNorm.y, prevNorm.x);

                StrokeVertex prev = outsidePrev;
                for (int s = 1; s <= steps; s++) {
                    const float a = startAngle + stepAngle * static_cast<float>(s);
                    const float2 dir = {std::cos(a), std::sin(a)};
                    const StrokeVertex curr = ss.emit(point + dir * halfStroke, color);
                    ss.emitTriangle(insidePrev, prev, curr);
                    prev = curr;
                }
                // Letztes Dreieck schliesst zum outsideNext
                ss.emitTriangle(insidePrev, prev, outsideNext);
                ss.emitTriangle(insidePrev, insideNext, outsideNext);
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
        return std::make_unique<LibTessTesselator>();
    }
} // namespace p5
