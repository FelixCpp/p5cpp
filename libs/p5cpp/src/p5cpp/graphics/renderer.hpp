#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/draw_state.hpp>
#include <p5cpp/graphics/pipeline_cache.hpp>
#include <p5cpp/graphics/texture_bind_group_cache.hpp>
#include <p5cpp/graphics/gpu_mesh_buffer.hpp>
#include <p5cpp/graphics/extra_uniforms_ring.hpp>

#include <webgpu/webgpu.h>

#include <optional>

namespace p5
{
    struct Graphics;
    class GpuDevice;

    struct RendererBatch
    {
        BlendMode blendMode;
        std::optional<rect2f> clipRect;
        TextureFilter textureFilter;
        TextureWrap textureWrap;
        Shader shader;
        Texture texture;
        ShaderUniformList uniforms;
        size_t indexOffset;
        size_t indexCount;
    };

    class Renderer
    {
    public:
        class Writer
        {
        public:
            void addVertex(const float2& position, const float2& texCoord, const float4& color);
            void addIndex(uint32_t index);

        private:
            friend class Renderer;
            Writer(Renderer& renderer, uint32_t vertexBase, size_t indexOffset);

            Renderer& m_renderer;
            uint32_t m_vertexBase;
            size_t m_indexOffset;
        };

        static std::unique_ptr<Renderer> create(GpuDevice& gpuDevice, size_t initialMaxVertices, size_t initialMaxIndices);
        ~Renderer();

        void begin(Graphics graphics);
        void end();
        void flush();

        Writer write();
        void finish(const Writer& writer, const BlendMode& blendMode, const std::optional<rect2f>& clipRect, TextureFilter textureFilter, TextureWrap textureWrap, const Texture& texture, const Shader& shader, const ShaderUniformList& uniforms);

    private:
        explicit Renderer(GpuDevice& gpuDevice, size_t maxVertexCount, size_t maxIndexCount);

        void appendVertex(const Vertex& vertex);
        void appendIndex(uint32_t index);
        void flushIfNearCapacity();

        GpuDevice& m_gpuDevice;

        // Bind-group layouts and the pipeline layout built from them must outlive every pipeline
        // and bind group created against them, so they stay here rather than moving into the
        // cache classes -- declared before those caches so C++'s reverse-declaration-order
        // destruction tears the caches down first.
        WGPUBindGroupLayout m_projectionLayout;
        WGPUBindGroupLayout m_textureLayout;
        WGPUBindGroupLayout m_extraUniformsLayout;
        WGPUPipelineLayout m_pipelineLayout;

        WGPUBuffer m_projectionBuffer;
        WGPUBindGroup m_projectionBindGroup;

        PipelineCache m_pipelineCache;
        TextureBindGroupCache m_textureBindGroupCache;
        ExtraUniformsRing m_extraUniformsRing;

        GpuMeshBuffer<Vertex> m_vertexMeshBuffer;
        GpuMeshBuffer<uint32_t> m_indexMeshBuffer;
        size_t m_currentVertexOffset;
        size_t m_currentIndexOffset;

        std::vector<RendererBatch> m_batches;
        matrix4x4 m_projectionMatrix;
        uint2 m_graphicsSize;
        Graphics m_graphics;
    };
} // namespace p5
