// #pragma once
//
// #include "vertex.hpp"
//
// #include <span>
//
// namespace p5cpp
// {
//     struct DrawScope
//     {
//         std::span<Vertex> vertices;
//         std::span<uint32_t> indices;
//         size_t& vertexCursor;
//         size_t& indexCursor;
//         size_t baseVertex;
//         size_t baseIndex;
//     };
//
//     void draw_scope_push_vertex(DrawScope& scope, const float2& position, const float2& texcoord, const float4& color);
//     void draw_scope_push_triangle(DrawScope& scope, uint32_t a, uint32_t b, uint32_t c);
// } // namespace p5cpp
