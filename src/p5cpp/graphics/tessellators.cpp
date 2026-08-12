#include <p5cpp/graphics/tessellators.hpp>

#include <tesselator.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <stdexcept>

namespace p5
{
    namespace
    {
        void addTriangle(VertexSink& sink, uint32_t a, uint32_t b, uint32_t c)
        {
            sink.addIndex(a);
            sink.addIndex(b);
            sink.addIndex(c);
        }

        void addVertices(VertexSink& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors)
        {
            for (size_t i = 0; i < positions.size(); ++i)
                sink.addVertex(positions[i], texCoords[i], colors[i]);
        }

        struct TessDeleter
        {
            void operator()(TESStesselator* tess) const
            {
                tessDeleteTess(tess);
            }
        };

        struct AttributeTessellation
        {
            std::vector<float> vertices;
            std::vector<TESSindex> elements;
        };

        AttributeTessellation tesselateAttribute(const std::span<const float2>& positions, const std::span<const float>& attribute, bool wantElements)
        {
            std::vector<float> contour(positions.size() * 3);
            for (size_t i = 0; i < positions.size(); ++i) {
                contour[i * 3 + 0] = positions[i].x;
                contour[i * 3 + 1] = positions[i].y;
                contour[i * 3 + 2] = attribute[i];
            }

            std::unique_ptr<TESStesselator, TessDeleter> tess(tessNewTess(nullptr));
            tessAddContour(tess.get(), 3, contour.data(), 3 * sizeof(float), static_cast<int>(positions.size()));

            AttributeTessellation result;
            if (not tessTesselate(tess.get(), TESS_WINDING_NONZERO, TESS_POLYGONS, 3, 3, nullptr))
                return result;

            const int vertexCount = tessGetVertexCount(tess.get());
            const float* vertices = tessGetVertices(tess.get());
            result.vertices.assign(vertices, vertices + vertexCount * 3);

            if (wantElements) {
                const int elementCount = tessGetElementCount(tess.get());
                const TESSindex* elements = tessGetElements(tess.get());
                result.elements.assign(elements, elements + elementCount * 3);
            }

            return result;
        }
    } // namespace

    void tesselate_triangle(VertexSink& sink, const std::span<const float2, 3>& positions, const std::span<const float2, 3>& texCoords, const std::span<const float4, 3>& colors)
    {
        addVertices(sink, positions, texCoords, colors);
        addTriangle(sink, 0, 1, 2);
    }

    void tesselate_triangles(VertexSink& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors)
    {
        const size_t count = positions.size();
        if (count % 3 != 0)
            throw std::runtime_error("tesselate_triangles: vertex count must be a multiple of 3");

        addVertices(sink, positions, texCoords, colors);

        for (size_t i = 0; i + 3 <= count; i += 3)
            addTriangle(sink, static_cast<uint32_t>(i), static_cast<uint32_t>(i + 1), static_cast<uint32_t>(i + 2));
    }

    void tesselate_triangle_strip(VertexSink& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors)
    {
        addVertices(sink, positions, texCoords, colors);

        const size_t count = positions.size();
        for (size_t i = 0; i + 2 < count; ++i) {
            if (i % 2 == 0)
                addTriangle(sink, static_cast<uint32_t>(i), static_cast<uint32_t>(i + 1), static_cast<uint32_t>(i + 2));
            else
                addTriangle(sink, static_cast<uint32_t>(i + 1), static_cast<uint32_t>(i), static_cast<uint32_t>(i + 2));
        }
    }

    void tesselate_triangle_fan(VertexSink& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors)
    {
        addVertices(sink, positions, texCoords, colors);

        const size_t count = positions.size();
        for (size_t i = 1; i + 1 < count; ++i)
            addTriangle(sink, 0, static_cast<uint32_t>(i), static_cast<uint32_t>(i + 1));
    }

    void tesselate_quad(VertexSink& sink, const std::span<const float2, 4>& positions, const std::span<const float2, 4>& texCoords, const std::span<const float4, 4>& colors)
    {
        addVertices(sink, positions, texCoords, colors);
        addTriangle(sink, 0, 1, 2);
        addTriangle(sink, 0, 2, 3);
    }

    void tesselate_quads(VertexSink& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors)
    {
        const size_t count = positions.size();
        if (count % 4 != 0)
            throw std::runtime_error("tesselate_quads: vertex count must be a multiple of 4");

        addVertices(sink, positions, texCoords, colors);

        for (size_t i = 0; i + 4 <= count; i += 4) {
            addTriangle(sink, static_cast<uint32_t>(i), static_cast<uint32_t>(i + 1), static_cast<uint32_t>(i + 2));
            addTriangle(sink, static_cast<uint32_t>(i), static_cast<uint32_t>(i + 2), static_cast<uint32_t>(i + 3));
        }
    }

    void tesselate_quad_strip(VertexSink& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors)
    {
        const size_t count = positions.size();
        if (count % 2 != 0)
            throw std::runtime_error("tesselate_quad_strip: vertex count must be even");

        addVertices(sink, positions, texCoords, colors);

        for (size_t i = 0; i + 4 <= count; i += 2) {
            addTriangle(sink, static_cast<uint32_t>(i), static_cast<uint32_t>(i + 1), static_cast<uint32_t>(i + 3));
            addTriangle(sink, static_cast<uint32_t>(i), static_cast<uint32_t>(i + 3), static_cast<uint32_t>(i + 2));
        }
    }

    void tesselate_polygon(VertexSink& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors)
    {
        if (positions.size() < 3)
            return;

        constexpr size_t channelCount = 6; // texCoord.x, texCoord.y, color.r, color.g, color.b, color.a
        const auto channelValue = [&](size_t channel, size_t vertexIndex) -> float {
            switch (channel) {
                case 0: return texCoords[vertexIndex].x;
                case 1: return texCoords[vertexIndex].y;
                case 2: return colors[vertexIndex].x;
                case 3: return colors[vertexIndex].y;
                case 4: return colors[vertexIndex].z;
                default: return colors[vertexIndex].w;
            }
        };

        std::vector<float> attribute(positions.size());
        std::array<AttributeTessellation, channelCount> channels;
        for (size_t channel = 0; channel < channelCount; ++channel) {
            for (size_t i = 0; i < positions.size(); ++i)
                attribute[i] = channelValue(channel, i);

            channels[channel] = tesselateAttribute(positions, attribute, channel == 0);
        }

        const size_t vertexCount = channels[0].vertices.size() / 3;
        for (size_t i = 0; i < vertexCount; ++i) {
            const float2 position {channels[0].vertices[i * 3 + 0], channels[0].vertices[i * 3 + 1]};
            const float2 texCoord {channels[0].vertices[i * 3 + 2], channels[1].vertices[i * 3 + 2]};
            const float4 color {channels[2].vertices[i * 3 + 2], channels[3].vertices[i * 3 + 2], channels[4].vertices[i * 3 + 2], channels[5].vertices[i * 3 + 2]};

            sink.addVertex(position, texCoord, color);
        }

        const std::vector<TESSindex>& elements = channels[0].elements;
        for (size_t i = 0; i + 2 < elements.size(); i += 3)
            addTriangle(sink, static_cast<uint32_t>(elements[i]), static_cast<uint32_t>(elements[i + 1]), static_cast<uint32_t>(elements[i + 2]));
    }
} // namespace p5

namespace p5
{
    namespace
    {
        struct PathPoint
        {
            float2 position;
            float2 texCoord;
            float4 color;
        };

        float2 subtract(const float2& a, const float2& b)
        {
            return {a.x - b.x, a.y - b.y};
        }

        float2 scale2(const float2& a, float s)
        {
            return {a.x * s, a.y * s};
        }

        float2 offsetBy(const float2& p, const float2& dir, float amount)
        {
            return {p.x + dir.x * amount, p.y + dir.y * amount};
        }

        float dot2(const float2& a, const float2& b)
        {
            return a.x * b.x + a.y * b.y;
        }

        float length2(const float2& a)
        {
            return std::sqrt(dot2(a, a));
        }

        float2 normalize2(const float2& a)
        {
            const float len = length2(a);
            return len > 1e-9f ? scale2(a, 1.0f / len) : float2 {0.0f, 0.0f};
        }

        // Rotates `a` by +90 degrees.
        float2 perpendicular(const float2& a)
        {
            return {-a.y, a.x};
        }

        uint32_t emitVertex(VertexSink& sink, uint32_t& vertexCount, const float2& position, const PathPoint& attrib)
        {
            sink.addVertex(position, attrib.texCoord, attrib.color);
            return vertexCount++;
        }

        void emitArcFan(VertexSink& sink, uint32_t& vertexCount, const PathPoint& center, uint32_t centerIndex, const float2& startPoint, float startAngle, float sweepAngle, float radius, float angleStep)
        {
            const int segments = std::max(1, static_cast<int>(std::ceil(std::abs(sweepAngle) / angleStep)));

            uint32_t prevIndex = emitVertex(sink, vertexCount, startPoint, center);
            for (int s = 1; s <= segments; ++s) {
                const float t = static_cast<float>(s) / static_cast<float>(segments);
                const float angle = startAngle + sweepAngle * t;
                const float2 rim = offsetBy(center.position, {std::cos(angle), std::sin(angle)}, radius);
                const uint32_t nextIndex = emitVertex(sink, vertexCount, rim, center);

                sink.addIndex(centerIndex);
                sink.addIndex(prevIndex);
                sink.addIndex(nextIndex);
                prevIndex = nextIndex;
            }
        }
    } // namespace

    void tesselate_path(VertexSink& sink, const std::span<const float2>& positions, const std::span<const float2>& texCoords, const std::span<const float4>& colors, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, float miterLimit, float roundJoinThreshold, bool closed)
    {
        if (strokeWeight <= 0.0f)
            return;

        std::vector<PathPoint> pts;
        pts.reserve(positions.size());
        for (size_t i = 0; i < positions.size(); ++i) {
            if (not pts.empty() && length2(subtract(positions[i], pts.back().position)) < 1e-9f)
                continue;
            pts.push_back({positions[i], texCoords[i], colors[i]});
        }

        const size_t pointCount = pts.size();
        if (closed ? pointCount < 3 : pointCount < 2)
            return;

        const float halfWidth = strokeWeight * 0.5f;

        const int fullCircleSegments = std::clamp(static_cast<int>(std::ceil(std::numbers::pi_v<float> * std::sqrt(2.0f * halfWidth))), 16, 256);
        const float arcAngleStep = 2.0f * std::numbers::pi_v<float> / static_cast<float>(fullCircleSegments);

        const size_t segmentCount = closed ? pointCount : pointCount - 1;

        const auto next = [pointCount](size_t i) {
            return (i + 1) % pointCount;
        };

        std::vector<float2> directions(segmentCount);
        std::vector<float2> normals(segmentCount);
        for (size_t i = 0; i < segmentCount; ++i) {
            directions[i] = normalize2(subtract(pts[next(i)].position, pts[i].position));
            normals[i] = perpendicular(directions[i]);
        }

        uint32_t vertexCount = 0;

        for (size_t i = 0; i < segmentCount; ++i) {
            const PathPoint& p0 = pts[i];
            const PathPoint& p1 = pts[next(i)];

            const uint32_t left0 = emitVertex(sink, vertexCount, offsetBy(p0.position, normals[i], halfWidth), p0);
            const uint32_t right0 = emitVertex(sink, vertexCount, offsetBy(p0.position, normals[i], -halfWidth), p0);
            const uint32_t left1 = emitVertex(sink, vertexCount, offsetBy(p1.position, normals[i], halfWidth), p1);
            const uint32_t right1 = emitVertex(sink, vertexCount, offsetBy(p1.position, normals[i], -halfWidth), p1);

            sink.addIndex(left0);
            sink.addIndex(right0);
            sink.addIndex(left1);

            sink.addIndex(right0);
            sink.addIndex(right1);
            sink.addIndex(left1);
        }

        const size_t jointBegin = closed ? 0 : 1;
        for (size_t i = jointBegin; i < segmentCount; ++i) {
            const size_t prevSeg = (i + segmentCount - 1) % segmentCount;
            const float2& dir0 = directions[prevSeg];
            const float2& dir1 = directions[i];
            const float cross = dir0.x * dir1.y - dir0.y * dir1.x;
            const float outerSign = cross > 0.0f ? -1.0f : 1.0f;

            const float2 outerN0 = scale2(normals[prevSeg], outerSign);
            const float2 outerN1 = scale2(normals[i], outerSign);

            const PathPoint& joint = pts[i];
            const float2 outerPrev = offsetBy(joint.position, outerN0, halfWidth);
            const float2 outerNext = offsetBy(joint.position, outerN1, halfWidth);
            const float2 innerPrev = offsetBy(joint.position, outerN0, -halfWidth);
            const float2 innerNext = offsetBy(joint.position, outerN1, -halfWidth);

            const uint32_t jointIndex = emitVertex(sink, vertexCount, joint.position, joint);
            const uint32_t innerPrevIndex = emitVertex(sink, vertexCount, innerPrev, joint);
            const uint32_t innerNextIndex = emitVertex(sink, vertexCount, innerNext, joint);
            sink.addIndex(jointIndex);
            sink.addIndex(innerPrevIndex);
            sink.addIndex(innerNextIndex);

            const auto emitOuterBevel = [&] {
                const uint32_t outerPrevIndex = emitVertex(sink, vertexCount, outerPrev, joint);
                const uint32_t outerNextIndex = emitVertex(sink, vertexCount, outerNext, joint);
                sink.addIndex(jointIndex);
                sink.addIndex(outerPrevIndex);
                sink.addIndex(outerNextIndex);
            };

            switch (strokeJoin) {
                case StrokeJoin::bevel:
                    emitOuterBevel();
                    break;

                case StrokeJoin::round: {
                    const float startAngle = std::atan2(outerN0.y, outerN0.x);
                    float sweep = std::atan2(outerN1.y, outerN1.x) - startAngle;
                    while (sweep <= -std::numbers::pi_v<float>) sweep += 2.0f * std::numbers::pi_v<float>;
                    while (sweep > std::numbers::pi_v<float>) sweep -= 2.0f * std::numbers::pi_v<float>;
                    if (std::abs(sweep) < roundJoinThreshold)
                        emitOuterBevel();
                    else
                        emitArcFan(sink, vertexCount, joint, jointIndex, outerPrev, startAngle, sweep, halfWidth, arcAngleStep);
                    break;
                }

                case StrokeJoin::miter:
                default: {
                    const float2 sum = {outerN0.x + outerN1.x, outerN0.y + outerN1.y};
                    const float sumLen = length2(sum);
                    bool didMiter = false;
                    if (sumLen > 1e-6f) {
                        const float2 miterDir = scale2(sum, 1.0f / sumLen);
                        const float cosHalf = dot2(miterDir, outerN0);
                        if (cosHalf > 1e-4f && halfWidth / cosHalf <= miterLimit * halfWidth) {
                            const float2 miterTip = offsetBy(joint.position, miterDir, halfWidth / cosHalf);
                            const uint32_t outerPrevIndex = emitVertex(sink, vertexCount, outerPrev, joint);
                            const uint32_t miterTipIndex = emitVertex(sink, vertexCount, miterTip, joint);
                            const uint32_t outerNextIndex = emitVertex(sink, vertexCount, outerNext, joint);
                            sink.addIndex(jointIndex);
                            sink.addIndex(outerPrevIndex);
                            sink.addIndex(miterTipIndex);
                            sink.addIndex(jointIndex);
                            sink.addIndex(miterTipIndex);
                            sink.addIndex(outerNextIndex);
                            didMiter = true;
                        }
                    }
                    if (not didMiter)
                        emitOuterBevel();
                    break;
                }
            }
        }

        const auto emitCap = [&](const PathPoint& endpoint, const float2& dir, StrokeCapStyle style, bool isStart) {
            if (style == StrokeCapStyle::butt)
                return;

            const float2 normal = perpendicular(dir);
            const float2 leftPoint = offsetBy(endpoint.position, normal, halfWidth);
            const float2 rightPoint = offsetBy(endpoint.position, normal, -halfWidth);
            const float2 outward = isStart ? scale2(dir, -1.0f) : dir;

            switch (style) {
                case StrokeCapStyle::butt:
                    break;

                case StrokeCapStyle::square: {
                    const float2 leftExt = offsetBy(leftPoint, outward, halfWidth);
                    const float2 rightExt = offsetBy(rightPoint, outward, halfWidth);
                    const uint32_t leftIndex = emitVertex(sink, vertexCount, leftPoint, endpoint);
                    const uint32_t rightIndex = emitVertex(sink, vertexCount, rightPoint, endpoint);
                    const uint32_t leftExtIndex = emitVertex(sink, vertexCount, leftExt, endpoint);
                    const uint32_t rightExtIndex = emitVertex(sink, vertexCount, rightExt, endpoint);
                    sink.addIndex(leftIndex);
                    sink.addIndex(rightIndex);
                    sink.addIndex(rightExtIndex);
                    sink.addIndex(leftIndex);
                    sink.addIndex(rightExtIndex);
                    sink.addIndex(leftExtIndex);
                    break;
                }

                case StrokeCapStyle::triangle: {
                    const float2 apex = offsetBy(endpoint.position, outward, halfWidth);
                    const uint32_t leftIndex = emitVertex(sink, vertexCount, leftPoint, endpoint);
                    const uint32_t rightIndex = emitVertex(sink, vertexCount, rightPoint, endpoint);
                    const uint32_t apexIndex = emitVertex(sink, vertexCount, apex, endpoint);
                    sink.addIndex(leftIndex);
                    sink.addIndex(rightIndex);
                    sink.addIndex(apexIndex);
                    break;
                }

                case StrokeCapStyle::round: {
                    const float startAngle = isStart ? std::atan2(normal.y, normal.x) : std::atan2(-normal.y, -normal.x);
                    const float2 startPoint = isStart ? leftPoint : rightPoint;
                    const uint32_t centerIndex = emitVertex(sink, vertexCount, endpoint.position, endpoint);
                    emitArcFan(sink, vertexCount, endpoint, centerIndex, startPoint, startAngle, std::numbers::pi_v<float>, halfWidth, arcAngleStep);
                    break;
                }
            }
        };

        if (not closed) {
            emitCap(pts.front(), directions.front(), strokeCap.start, true);
            emitCap(pts.back(), directions.back(), strokeCap.end, false);
        }
    }
} // namespace p5
