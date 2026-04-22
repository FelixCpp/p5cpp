#include "geometry.hpp"
#include "tesselator.h"

#include <cassert>
#include <vector>

namespace p5
{
    static float4 color_to_float4(color_t color)
    {
        return float4 {
            .x = static_cast<float>(red(color)) / 255.0f,
            .y = static_cast<float>(green(color)) / 255.0f,
            .z = static_cast<float>(blue(color)) / 255.0f,
            .w = static_cast<float>(alpha(color)) / 255.0f,
        };
    }

    void GeometryBuilder::append(Vertex&& vertex)
    {
        assert((initialVertexOffset + vertexCount + 1 <= buffer->vertexCount) && "Exceeded vertex limit");

        buffer->vertices[initialVertexOffset + vertexCount] = std::move(vertex);
        vertexCount += 1;
    }

    void GeometryBuilder::addTriangle(uint32_t a, uint32_t b, uint32_t c)
    {
        assert((a < vertexCount) and (b < vertexCount) and (c < vertexCount) and "Exceeded index limit");

        buffer->indices[initialIndexOffset + indexCount + 0] = initialVertexOffset + a;
        buffer->indices[initialIndexOffset + indexCount + 1] = initialVertexOffset + b;
        buffer->indices[initialIndexOffset + indexCount + 2] = initialVertexOffset + c;

        indexCount += 3;
    }

    void GeometryBuilder::convex(const std::span<const GeometryPoint>& vertices)
    {
        for (size_t i = 0; i < vertices.size(); ++i) {
            const GeometryPoint& point = vertices[i];

            append(Vertex {
                .position = float3 {point.position.x, point.position.y, 0.0f},
                .texcoord = point.texcoord,
                .color = color_to_float4(point.fillColor),
            });
        }

        for (size_t i = 1; i < vertices.size() - 1; ++i) {
            addTriangle(0, i, i + 1);
        }
    }

    void GeometryBuilder::concave(const std::span<const GeometryPoint>& points)
    {
        std::vector<float> positions;

        for (const GeometryPoint& p : points) {
            positions.push_back(p.position.x);
            positions.push_back(p.position.y);
            positions.push_back(0.0f);
        }

        TESStesselator* tess = tessNewTess(nullptr);
        tessAddContour(tess, 3, positions.data(), sizeof(float) * 3, positions.size());
        tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 3, nullptr);

        const float* tessVertices = tessGetVertices(tess);
        const int* tessIndices = tessGetElements(tess);
        const int* vertexIndex = tessGetVertexIndices(tess);
        const int vertexCount = tessGetVertexCount(tess);
        const int indexCount = tessGetElementCount(tess);

        // tesselator->addContour(positions);

        // if (not tesselator->tesselate()) {
        //     return;
        // }
        //
        // const auto tessVertices = tesselator->vertices();
        // const auto tessIndices = tesselator->indices();
        // const auto vertexCount = tesselator->getVertexCount();
        // const auto indexCount = tesselator->getElementCount();
        // const auto vertexIndex = tesselator->getVertexIndices();

        for (int i = 0; i < vertexCount; ++i) {
            const int srcIdx = vertexIndex[i];

            if (srcIdx == TESS_UNDEF) {
                float2 pos = {
                    .x = tessVertices[i * 3 + 0],
                    .y = tessVertices[i * 3 + 1],
                };

                append(Vertex {
                    .position = float3 {.x = pos.x, .y = pos.y, .z = 0.0f},
                    .texcoord = float2 {.x = 0.0f, .y = 0.0f},
                    .color = color_to_float4(points[0].fillColor),
                });
            } else {
                const GeometryPoint& src = points[srcIdx];

                append(Vertex {
                    .position = float3 {.x = src.position.x, .y = src.position.y, .z = 0.0f},
                    .texcoord = src.texcoord,
                    .color = color_to_float4(src.fillColor),
                });
            }
        }

        for (int i = 0; i < indexCount; ++i) {
            const int a = tessIndices[i * 3 + 0];
            const int b = tessIndices[i * 3 + 1];
            const int c = tessIndices[i * 3 + 2];

            if (a == TESS_UNDEF or b == TESS_UNDEF or c == TESS_UNDEF) continue;
            addTriangle(a, b, c);
        }

        tessDeleteTess(tess);
    }

    void GeometryBuilder::stroke(const std::span<const GeometryPoint>& vertices, float strokeWeight, StrokeCap strokeCap, StrokeJoin strokeJoin, StrokeAlign strokeAlign, bool close)
    {
    }
} // namespace p5
