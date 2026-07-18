#pragma once

#include <p5cpp/graphics/shader.hpp>
#include <p5cpp/graphics/texture.hpp>
#include <p5cpp/graphics/blendmode.hpp>
#include <p5cpp/graphics/draw_buffer_writer.hpp>
#include <p5cpp/graphics/uniform_cache.hpp>
#include <p5cpp/math/value2.hpp>
#include <p5cpp/math/value4.hpp>

#include <cstdint>
#include <utility>
#include <vector>

namespace p5cpp
{
    struct RecordedVertex
    {
        float2 position; // local-space: relative to the buildRenderGroup() call's own identity-based matrix stack
        float2 texcoord;
        float4 color;
    };

    // One fill or stroke submission recorded during buildRenderGroup(), already
    // tessellated/stroked into triangles — replaying it costs a per-vertex transform
    // and a submit() call, not another pass through libtess2/the stroker.
    struct RecordedOp
    {
        std::vector<RecordedVertex> vertices;
        std::vector<uint32_t> indices; // triples, 0-based relative to this op's own vertices
        Shader shader;
        BlendMode blendMode;
        Texture texture;
        std::vector<UniformSnapshot> uniforms; // frozen at build time, independent of the live UniformCache
    };

    struct RenderGroupImpl
    {
        std::vector<RecordedOp> ops;
    };

    // DrawBufferWriter that appends into an in-progress op's scratch buffers instead of
    // the live renderer's GPU-bound staging buffer. Indices are recorded relative to this
    // op alone (0-based), matching what tess.cpp/stroker.cpp already produce by calling
    // getRelativeCursor() fresh at the start of each shape.
    class RecordingDrawBufferWriter final : public DrawBufferWriter
    {
    public:
        uint32_t getRelativeCursor() const override
        {
            return static_cast<uint32_t>(vertices.size());
        }

        void pushVertex(const float2& position, const float2& texcoord, const float4& color) override
        {
            vertices.push_back(RecordedVertex {position, texcoord, color});
        }

        void pushTriangle(uint32_t a, uint32_t b, uint32_t c) override
        {
            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(c);
        }

        std::vector<RecordedVertex> vertices;
        std::vector<uint32_t> indices;
    };

    // One buildRenderGroup() call's in-progress state: a scratch writer for whichever op
    // is currently being generated, plus every op committed so far, in call order.
    struct RecordingSink
    {
        RecordingDrawBufferWriter writer;
        std::vector<RecordedOp> ops;

        void commit(const Shader& shader, const BlendMode& blendMode, const Texture& texture, std::vector<UniformSnapshot> uniforms)
        {
            if (!writer.indices.empty()) {
                ops.push_back(RecordedOp {
                    std::move(writer.vertices),
                    std::move(writer.indices),
                    shader,
                    blendMode,
                    texture,
                    std::move(uniforms),
                });
            }

            writer.vertices.clear();
            writer.indices.clear();
        }
    };
} // namespace p5cpp
