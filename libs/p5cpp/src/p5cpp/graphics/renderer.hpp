#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/vertex_sink.hpp>

#include <webgpu/webgpu.h>

#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace p5
{
    struct Graphics;

    struct RendererBatch
    {
        BlendMode blendMode;
        std::optional<rect2f> clipRect;
        TextureFilter textureFilter;
        TextureWrap textureWrap;
        Shader shader;
        Texture texture;
        std::unordered_map<std::string, UniformValue> uniforms;
        size_t indexOffset;
        size_t indexCount;
    };

    class Renderer
    {
    public:
        class Writer : public VertexSink
        {
        public:
            void addVertex(const float2& position, const float2& texCoord, const float4& color) override;
            void addIndex(uint32_t index) override;

        private:
            friend class Renderer;
            Writer(Renderer& renderer, uint32_t vertexBase, size_t indexOffset);

            Renderer& m_renderer;
            uint32_t m_vertexBase;
            size_t m_indexOffset;
        };

        static std::unique_ptr<Renderer> create(size_t initialMaxVertices, size_t initialMaxIndices);
        ~Renderer();

        void begin(Graphics graphics);
        void end();
        void flush();

        Writer write();
        void finish(const Writer& writer, const BlendMode& blendMode, const std::optional<rect2f>& clipRect, TextureFilter textureFilter, TextureWrap textureWrap, const Texture& texture, const Shader& shader, const std::unordered_map<std::string, UniformValue>& uniforms);

    private:
        struct PipelineKey
        {
            ShaderImpl* shader;
            BlendMode blendMode;
            uint32_t sampleCount;
            WGPUTextureFormat colorFormat;

            bool operator<(const PipelineKey& other) const;
        };
        struct PipelineEntry
        {
            Shader shader;
            WGPURenderPipeline pipeline;
        };

        struct TextureBindGroupKey
        {
            TextureImpl* texture;
            TextureFilter filter;
            TextureWrap wrap;

            bool operator<(const TextureBindGroupKey& other) const;
        };
        struct TextureBindGroupEntry
        {
            Texture texture;
            WGPUBindGroup bindGroup;
        };

        explicit Renderer(WGPUBuffer vertexBuffer, WGPUBuffer indexBuffer, size_t maxVertexCount, size_t maxIndexCount);

        void appendVertex(const Vertex& vertex);
        void appendIndex(uint32_t index);
        void flushIfNearCapacity();

        WGPURenderPipeline getOrCreatePipeline(const Shader& shader, const BlendMode& blendMode, uint32_t sampleCount, WGPUTextureFormat colorFormat);
        WGPUBindGroup getOrCreateTextureBindGroup(const Texture& texture, TextureFilter filter, TextureWrap wrap);
        WGPUSampler getOrCreateSampler(TextureFilter filter, TextureWrap wrap);

        uint32_t writeExtraUniforms(const RendererBatch& batch);

        WGPUBuffer m_vertexBuffer;
        WGPUBuffer m_indexBuffer;

        WGPUBindGroupLayout m_projectionLayout;
        WGPUBindGroupLayout m_textureLayout;
        WGPUBindGroupLayout m_extraUniformsLayout;
        WGPUPipelineLayout m_pipelineLayout;

        WGPUBuffer m_projectionBuffer;
        WGPUBindGroup m_projectionBindGroup;

        WGPUBuffer m_extraUniformsBuffer;
        WGPUBindGroup m_extraUniformsBindGroup;
        size_t m_extraUniformsBufferCapacity;
        size_t m_extraUniformsWriteCursor;

        std::map<PipelineKey, PipelineEntry> m_pipelines;
        std::map<TextureBindGroupKey, TextureBindGroupEntry> m_textureBindGroups;
        std::map<std::pair<TextureFilter, TextureWrap>, WGPUSampler> m_samplers;

        std::vector<Vertex> m_vertices;
        std::vector<uint32_t> m_indices;
        size_t m_uploadedVertexCapacity;
        size_t m_uploadedIndexCapacity;

        size_t m_currentVertexOffset;
        size_t m_currentIndexOffset;

        std::vector<RendererBatch> m_batches;
        matrix4x4 m_projectionMatrix;
        uint2 m_graphicsSize;
        Graphics m_graphics;
    };
} // namespace p5
