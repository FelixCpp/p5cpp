#include <p5cpp/graphics/renderer.hpp>
#include <p5cpp/graphics/texture_impl.hpp>
#include <p5cpp/graphics/shader_impl.hpp>
#include <p5cpp/graphics/graphics_impl.hpp>
#include <p5cpp/graphics/gpu_device.hpp>
#include <p5cpp/graphics/gpu_command.hpp>
#include <p5cpp/graphics/wgsl_conventions.hpp>

#include <webgpu/webgpu.h>

#include <algorithm>
#include <cassert>
#include <cstddef>

namespace p5
{
    namespace
    {
        constexpr uint32_t kExtraUniformsSlotStride = 256;
        constexpr uint32_t kExtraUniformsBufferCapacity = kExtraUniformsSlotStride * 1024;

        matrix4x4 transposed(const matrix4x4& m)
        {
            matrix4x4 result;
            for (int row = 0; row < 4; ++row) {
                for (int col = 0; col < 4; ++col) {
                    result.m[col * 4 + row] = m.m[row * 4 + col];
                }
            }
            return result;
        }

        WGPUBindGroupLayout createProjectionLayout(WGPUDevice device)
        {
            WGPUBindGroupLayoutEntry entry {};
            entry.binding = 0;
            entry.visibility = WGPUShaderStage_Vertex;
            entry.buffer.type = WGPUBufferBindingType_Uniform;
            entry.buffer.minBindingSize = sizeof(matrix4x4);

            WGPUBindGroupLayoutDescriptor desc {};
            desc.entryCount = 1;
            desc.entries = &entry;
            return wgpuDeviceCreateBindGroupLayout(device, &desc);
        }

        WGPUBindGroupLayout createTextureLayout(WGPUDevice device)
        {
            WGPUBindGroupLayoutEntry entries[2] = {};
            entries[0].binding = 0;
            entries[0].visibility = WGPUShaderStage_Fragment;
            entries[0].texture.sampleType = WGPUTextureSampleType_Float;
            entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
            entries[1].binding = 1;
            entries[1].visibility = WGPUShaderStage_Fragment;
            entries[1].sampler.type = WGPUSamplerBindingType_Filtering;

            WGPUBindGroupLayoutDescriptor desc {};
            desc.entryCount = 2;
            desc.entries = entries;
            return wgpuDeviceCreateBindGroupLayout(device, &desc);
        }

        WGPUBindGroupLayout createExtraUniformsLayout(WGPUDevice device)
        {
            WGPUBindGroupLayoutEntry entry {};
            entry.binding = 0;
            entry.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
            entry.buffer.type = WGPUBufferBindingType_Uniform;
            entry.buffer.hasDynamicOffset = true;

            WGPUBindGroupLayoutDescriptor desc {};
            desc.entryCount = 1;
            desc.entries = &entry;
            return wgpuDeviceCreateBindGroupLayout(device, &desc);
        }

        WGPUPipelineLayout createPipelineLayout(WGPUDevice device, WGPUBindGroupLayout projectionLayout, WGPUBindGroupLayout textureLayout, WGPUBindGroupLayout extraUniformsLayout)
        {
            WGPUBindGroupLayout layouts[3] = {projectionLayout, textureLayout, extraUniformsLayout};
            WGPUPipelineLayoutDescriptor desc {};
            desc.bindGroupLayoutCount = 3;
            desc.bindGroupLayouts = layouts;
            return wgpuDeviceCreatePipelineLayout(device, &desc);
        }

        WGPUBuffer createProjectionBuffer(WGPUDevice device)
        {
            WGPUBufferDescriptor desc {};
            desc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
            desc.size = sizeof(matrix4x4);
            return wgpuDeviceCreateBuffer(device, &desc);
        }

        WGPUBindGroup createProjectionBindGroup(WGPUDevice device, WGPUBindGroupLayout layout, WGPUBuffer buffer)
        {
            WGPUBindGroupEntry entry {};
            entry.binding = 0;
            entry.buffer = buffer;
            entry.size = sizeof(matrix4x4);

            WGPUBindGroupDescriptor desc {};
            desc.layout = layout;
            desc.entryCount = 1;
            desc.entries = &entry;
            return wgpuDeviceCreateBindGroup(device, &desc);
        }
    } // namespace

    std::unique_ptr<Renderer> Renderer::create(GpuDevice& gpuDevice, size_t initialMaxVertices, size_t initialMaxIndices)
    {
        return std::unique_ptr<Renderer>(new Renderer(gpuDevice, initialMaxVertices, initialMaxIndices));
    }

    Renderer::Renderer(GpuDevice& gpuDevice, size_t maxVertexCount, size_t maxIndexCount)
        : m_gpuDevice(gpuDevice),
          m_projectionLayout(createProjectionLayout(gpuDevice.getDevice())),
          m_textureLayout(createTextureLayout(gpuDevice.getDevice())),
          m_extraUniformsLayout(createExtraUniformsLayout(gpuDevice.getDevice())),
          m_pipelineLayout(createPipelineLayout(gpuDevice.getDevice(), m_projectionLayout, m_textureLayout, m_extraUniformsLayout)),
          m_projectionBuffer(createProjectionBuffer(gpuDevice.getDevice())),
          m_projectionBindGroup(createProjectionBindGroup(gpuDevice.getDevice(), m_projectionLayout, m_projectionBuffer)),
          m_pipelineCache(gpuDevice, m_pipelineLayout),
          m_textureBindGroupCache(gpuDevice, m_textureLayout),
          m_extraUniformsRing(gpuDevice, m_extraUniformsLayout, kExtraUniformsSlotStride, kExtraUniformsBufferCapacity),
          m_vertexMeshBuffer(gpuDevice, WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst, maxVertexCount),
          m_indexMeshBuffer(gpuDevice, WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst, maxIndexCount),
          m_currentVertexOffset(0),
          m_currentIndexOffset(0)
    {
    }

    Renderer::~Renderer()
    {
        wgpuBindGroupRelease(m_projectionBindGroup);
        wgpuBufferDestroy(m_projectionBuffer);
        wgpuBufferRelease(m_projectionBuffer);

        wgpuPipelineLayoutRelease(m_pipelineLayout);
        wgpuBindGroupLayoutRelease(m_extraUniformsLayout);
        wgpuBindGroupLayoutRelease(m_textureLayout);
        wgpuBindGroupLayoutRelease(m_projectionLayout);
    }

    void Renderer::begin(Graphics graphics)
    {
        if (not graphics.isValid()) {
            error("Renderer::begin() called with invalid graphics");
            return;
        }

        m_graphics = graphics;

        const uint2& size = graphics.size;
        m_projectionMatrix = orthographicProjectionMatrix(0.0f, 0.0f, static_cast<float>(size.x), static_cast<float>(size.y), -1.0f, 1.0f);
        m_graphicsSize = size;

        const matrix4x4 wgslProjectionMatrix = transposed(m_projectionMatrix);
        wgpuQueueWriteBuffer(m_gpuDevice.getQueue(), m_projectionBuffer, 0, wgslProjectionMatrix.m.data(), sizeof(matrix4x4));

        m_currentVertexOffset = 0;
        m_currentIndexOffset = 0;
        m_batches.clear();
    }

    void Renderer::end()
    {
        flush();
    }

    void Renderer::flush()
    {
        if (m_currentIndexOffset == 0)
            return;

        WGPUDevice device = m_gpuDevice.getDevice();
        WGPUQueue queue = m_gpuDevice.getQueue();

        m_vertexMeshBuffer.upload(queue, m_currentVertexOffset);
        m_indexMeshBuffer.upload(queue, m_currentIndexOffset);
        m_extraUniformsRing.resetForFrame();

        const bool hasMsaa = m_graphics.impl->msaaView != nullptr;
        const uint32_t sampleCount = hasMsaa ? 4 : 1;
        const std::optional<WGPUTextureFormat> colorFormat = toWGPUTextureFormat(m_graphics.colorTexture.pixelFormat);
        assert(colorFormat.has_value());

        WGPURenderPassColorAttachment colorAttachment {};
        colorAttachment.view = hasMsaa ? m_graphics.impl->msaaView : m_graphics.colorTexture.impl->view;
        colorAttachment.resolveTarget = hasMsaa ? m_graphics.colorTexture.impl->view : nullptr;
        colorAttachment.loadOp = WGPULoadOp_Load;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

        WGPURenderPassDescriptor passDescriptor {};
        passDescriptor.colorAttachmentCount = 1;
        passDescriptor.colorAttachments = &colorAttachment;

        GpuCommandScope commands(device);
        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(commands.encoder(), &passDescriptor);

        wgpuRenderPassEncoderSetVertexBuffer(pass, 0, m_vertexMeshBuffer.buffer(), 0, m_currentVertexOffset * sizeof(Vertex));
        wgpuRenderPassEncoderSetIndexBuffer(pass, m_indexMeshBuffer.buffer(), WGPUIndexFormat_Uint32, 0, m_currentIndexOffset * sizeof(uint32_t));
        wgpuRenderPassEncoderSetBindGroup(pass, wgsl::kProjectionBindGroup, m_projectionBindGroup, 0, nullptr);

        WGPURenderPipeline currentPipeline = nullptr;

        for (const RendererBatch& batch : m_batches) {
            WGPURenderPipeline pipeline = m_pipelineCache.getOrCreate(batch.shader, batch.blendMode, sampleCount, *colorFormat);
            if (pipeline == nullptr) {
                continue;
            }
            if (pipeline != currentPipeline) {
                wgpuRenderPassEncoderSetPipeline(pass, pipeline);
                currentPipeline = pipeline;
            }

            if (batch.clipRect.has_value()) {
                const float targetWidth = static_cast<float>(m_graphicsSize.x);
                const float targetHeight = static_cast<float>(m_graphicsSize.y);
                const float left = std::clamp(batch.clipRect->left, 0.0f, targetWidth);
                const float top = std::clamp(batch.clipRect->top, 0.0f, targetHeight);
                const float right = std::clamp(batch.clipRect->left + std::max(batch.clipRect->width, 0.0f), 0.0f, targetWidth);
                const float bottom = std::clamp(batch.clipRect->top + std::max(batch.clipRect->height, 0.0f), 0.0f, targetHeight);
                wgpuRenderPassEncoderSetScissorRect(
                    pass,
                    static_cast<uint32_t>(left),
                    static_cast<uint32_t>(top),
                    static_cast<uint32_t>(std::max(right - left, 0.0f)),
                    static_cast<uint32_t>(std::max(bottom - top, 0.0f))
                );
            } else {
                wgpuRenderPassEncoderSetScissorRect(pass, 0, 0, m_graphicsSize.x, m_graphicsSize.y);
            }

            WGPUBindGroup textureBindGroup = m_textureBindGroupCache.getOrCreate(batch.texture, batch.textureFilter, batch.textureWrap);
            wgpuRenderPassEncoderSetBindGroup(pass, wgsl::kTextureBindGroup, textureBindGroup, 0, nullptr);

            const uint32_t extraUniformsOffset = m_extraUniformsRing.write(batch);
            wgpuRenderPassEncoderSetBindGroup(pass, wgsl::kExtraUniformsBindGroup, m_extraUniformsRing.bindGroup(), 1, &extraUniformsOffset);

            wgpuRenderPassEncoderDrawIndexed(pass, static_cast<uint32_t>(batch.indexCount), 1, static_cast<uint32_t>(batch.indexOffset), 0, 0);
        }

        wgpuRenderPassEncoderEnd(pass);
        wgpuRenderPassEncoderRelease(pass);

        commands.submit(queue);

        m_pipelineCache.evictUnused();
        m_textureBindGroupCache.evictUnused();

        m_currentVertexOffset = 0;
        m_currentIndexOffset = 0;
        m_batches.clear();
    }

    void Renderer::appendVertex(const Vertex& vertex)
    {
        m_vertexMeshBuffer.ensureCapacityFor(m_currentVertexOffset);
        m_vertexMeshBuffer.data()[m_currentVertexOffset++] = vertex;
    }

    void Renderer::appendIndex(uint32_t index)
    {
        m_indexMeshBuffer.ensureCapacityFor(m_currentIndexOffset);
        m_indexMeshBuffer.data()[m_currentIndexOffset++] = index;
    }

    void Renderer::flushIfNearCapacity()
    {
        const size_t maxVertexCount = m_vertexMeshBuffer.data().size();
        const size_t maxIndexCount = m_indexMeshBuffer.data().size();
        const bool lowOnVertices = m_currentVertexOffset > 0 and (maxVertexCount - m_currentVertexOffset) < maxVertexCount / 4;
        const bool lowOnIndices = m_currentIndexOffset > 0 and (maxIndexCount - m_currentIndexOffset) < maxIndexCount / 4;
        if (lowOnVertices or lowOnIndices)
            flush();
    }

    Renderer::Writer Renderer::write()
    {
        flushIfNearCapacity();

        assert(m_currentVertexOffset <= std::numeric_limits<uint32_t>::max());
        return Writer(*this, static_cast<uint32_t>(m_currentVertexOffset), m_currentIndexOffset);
    }

    void Renderer::finish(const Writer& writer, const BlendMode& blendMode, const std::optional<rect2f>& clipRect, TextureFilter textureFilter, TextureWrap textureWrap, const Texture& texture, const Shader& shader, const ShaderUniformList& uniforms)
    {
        const size_t indexCount = m_currentIndexOffset - writer.m_indexOffset;
        if (indexCount == 0)
            return;

        if (not m_batches.empty()) {
            RendererBatch& lastBatch = m_batches.back();
            if (lastBatch.blendMode == blendMode and lastBatch.clipRect == clipRect and lastBatch.textureFilter == textureFilter and lastBatch.textureWrap == textureWrap and lastBatch.shader == shader and lastBatch.texture == texture and lastBatch.uniforms == uniforms) {
                lastBatch.indexCount += indexCount;
                return;
            }
        }

        m_batches.push_back(RendererBatch {
            .blendMode = blendMode,
            .clipRect = clipRect,
            .textureFilter = textureFilter,
            .textureWrap = textureWrap,
            .shader = shader,
            .texture = texture,
            .uniforms = uniforms,
            .indexOffset = writer.m_indexOffset,
            .indexCount = indexCount,
        });
    }

    Renderer::Writer::Writer(Renderer& renderer, uint32_t vertexBase, size_t indexOffset)
        : m_renderer(renderer),
          m_vertexBase(vertexBase),
          m_indexOffset(indexOffset)
    {
    }

    void Renderer::Writer::addVertex(const float2& position, const float2& texCoord, const float4& color)
    {
        m_renderer.appendVertex(Vertex {.position = position, .texCoord = texCoord, .color = color});
    }

    void Renderer::Writer::addIndex(uint32_t index)
    {
        m_renderer.appendIndex(m_vertexBase + index);
    }
} // namespace p5
