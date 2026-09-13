#include <p5cpp/graphics/renderer.hpp>
#include <p5cpp/graphics/texture_impl.hpp>
#include <p5cpp/graphics/shader_impl.hpp>
#include <p5cpp/graphics/graphics_impl.hpp>
#include <p5cpp/graphics/gpu_device.hpp>

#include <webgpu/webgpu.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <limits>
#include <type_traits>

namespace p5
{
    namespace
    {
        constexpr uint32_t kExtraUniformsSlotStride = 256;
        constexpr uint32_t kExtraUniformsBufferCapacity = kExtraUniformsSlotStride * 1024;

        WGPUBlendFactor toWgpu(BlendMode::Factor factor)
        {
            switch (factor) {
                case BlendMode::Factor::zero: return WGPUBlendFactor_Zero;
                case BlendMode::Factor::one: return WGPUBlendFactor_One;
                case BlendMode::Factor::srcColor: return WGPUBlendFactor_Src;
                case BlendMode::Factor::oneMinusSrcColor: return WGPUBlendFactor_OneMinusSrc;
                case BlendMode::Factor::dstColor: return WGPUBlendFactor_Dst;
                case BlendMode::Factor::oneMinusDstColor: return WGPUBlendFactor_OneMinusDst;
                case BlendMode::Factor::srcAlpha: return WGPUBlendFactor_SrcAlpha;
                case BlendMode::Factor::oneMinusSrcAlpha: return WGPUBlendFactor_OneMinusSrcAlpha;
                case BlendMode::Factor::dstAlpha: return WGPUBlendFactor_DstAlpha;
                case BlendMode::Factor::oneMinusDstAlpha: return WGPUBlendFactor_OneMinusDstAlpha;
                default:
                    error("Renderer: invalid BlendMode::Factor, falling back to one");
                    return WGPUBlendFactor_One;
            }
        }

        WGPUBlendOperation toWgpu(BlendMode::Equation equation)
        {
            switch (equation) {
                case BlendMode::Equation::add: return WGPUBlendOperation_Add;
                case BlendMode::Equation::subtract: return WGPUBlendOperation_Subtract;
                case BlendMode::Equation::reverseSubtract: return WGPUBlendOperation_ReverseSubtract;
                case BlendMode::Equation::min: return WGPUBlendOperation_Min;
                case BlendMode::Equation::max: return WGPUBlendOperation_Max;
                default:
                    error("Renderer: invalid BlendMode::Equation, falling back to add");
                    return WGPUBlendOperation_Add;
            }
        }

        WGPUFilterMode toWgpu(TextureFilter filter)
        {
            switch (filter) {
                case TextureFilter::nearest: return WGPUFilterMode_Nearest;
                case TextureFilter::linear: return WGPUFilterMode_Linear;
                default:
                    error("Renderer: invalid TextureFilter, falling back to nearest");
                    return WGPUFilterMode_Nearest;
            }
        }

        WGPUAddressMode toWgpu(TextureWrap wrap)
        {
            switch (wrap) {
                case TextureWrap::repeat: return WGPUAddressMode_Repeat;
                case TextureWrap::mirroredRepeat: return WGPUAddressMode_MirrorRepeat;
                case TextureWrap::clampToEdge: return WGPUAddressMode_ClampToEdge;
                default:
                    error("Renderer: invalid TextureWrap, falling back to clampToEdge");
                    return WGPUAddressMode_ClampToEdge;
            }
        }

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
    } // namespace

    bool Renderer::PipelineKey::operator<(const PipelineKey& other) const
    {
        return std::tie(shader, blendMode.srcColorFactor, blendMode.dstColorFactor, blendMode.colorEquation, blendMode.srcAlphaFactor, blendMode.dstAlphaFactor, blendMode.alphaEquation, sampleCount, colorFormat)
             < std::tie(other.shader, other.blendMode.srcColorFactor, other.blendMode.dstColorFactor, other.blendMode.colorEquation, other.blendMode.srcAlphaFactor, other.blendMode.dstAlphaFactor, other.blendMode.alphaEquation, other.sampleCount, other.colorFormat);
    }

    bool Renderer::TextureBindGroupKey::operator<(const TextureBindGroupKey& other) const
    {
        return std::tie(texture, filter, wrap) < std::tie(other.texture, other.filter, other.wrap);
    }

    std::unique_ptr<Renderer> Renderer::create(size_t initialMaxVertices, size_t initialMaxIndices)
    {
        GpuDevice& gpuDevice = requireDependency<GpuDevice>();
        WGPUDevice device = gpuDevice.getDevice();

        WGPUBufferDescriptor vertexBufferDesc {};
        vertexBufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        vertexBufferDesc.size = initialMaxVertices * sizeof(Vertex);
        WGPUBuffer vertexBuffer = wgpuDeviceCreateBuffer(device, &vertexBufferDesc);

        WGPUBufferDescriptor indexBufferDesc {};
        indexBufferDesc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
        indexBufferDesc.size = initialMaxIndices * sizeof(uint32_t);
        WGPUBuffer indexBuffer = wgpuDeviceCreateBuffer(device, &indexBufferDesc);

        return std::unique_ptr<Renderer>(new Renderer(vertexBuffer, indexBuffer, initialMaxVertices, initialMaxIndices));
    }

    Renderer::Renderer(WGPUBuffer vertexBuffer, WGPUBuffer indexBuffer, size_t maxVertexCount, size_t maxIndexCount)
        : m_vertexBuffer(vertexBuffer),
          m_indexBuffer(indexBuffer),
          m_extraUniformsBufferCapacity(kExtraUniformsBufferCapacity),
          m_extraUniformsWriteCursor(0),
          m_vertices(maxVertexCount),
          m_indices(maxIndexCount),
          m_uploadedVertexCapacity(maxVertexCount),
          m_uploadedIndexCapacity(maxIndexCount),
          m_currentVertexOffset(0),
          m_currentIndexOffset(0)
    {
        GpuDevice& gpuDevice = requireDependency<GpuDevice>();
        WGPUDevice device = gpuDevice.getDevice();

        WGPUBindGroupLayoutEntry projectionEntry {};
        projectionEntry.binding = 0;
        projectionEntry.visibility = WGPUShaderStage_Vertex;
        projectionEntry.buffer.type = WGPUBufferBindingType_Uniform;
        projectionEntry.buffer.minBindingSize = sizeof(matrix4x4);

        WGPUBindGroupLayoutDescriptor projectionLayoutDesc {};
        projectionLayoutDesc.entryCount = 1;
        projectionLayoutDesc.entries = &projectionEntry;
        m_projectionLayout = wgpuDeviceCreateBindGroupLayout(device, &projectionLayoutDesc);

        WGPUBindGroupLayoutEntry textureEntries[2] = {};
        textureEntries[0].binding = 0;
        textureEntries[0].visibility = WGPUShaderStage_Fragment;
        textureEntries[0].texture.sampleType = WGPUTextureSampleType_Float;
        textureEntries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
        textureEntries[1].binding = 1;
        textureEntries[1].visibility = WGPUShaderStage_Fragment;
        textureEntries[1].sampler.type = WGPUSamplerBindingType_Filtering;

        WGPUBindGroupLayoutDescriptor textureLayoutDesc {};
        textureLayoutDesc.entryCount = 2;
        textureLayoutDesc.entries = textureEntries;
        m_textureLayout = wgpuDeviceCreateBindGroupLayout(device, &textureLayoutDesc);

        WGPUBindGroupLayoutEntry extraUniformsEntry {};
        extraUniformsEntry.binding = 0;
        extraUniformsEntry.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
        extraUniformsEntry.buffer.type = WGPUBufferBindingType_Uniform;
        extraUniformsEntry.buffer.hasDynamicOffset = true;

        WGPUBindGroupLayoutDescriptor extraUniformsLayoutDesc {};
        extraUniformsLayoutDesc.entryCount = 1;
        extraUniformsLayoutDesc.entries = &extraUniformsEntry;
        m_extraUniformsLayout = wgpuDeviceCreateBindGroupLayout(device, &extraUniformsLayoutDesc);

        WGPUBindGroupLayout layouts[3] = {m_projectionLayout, m_textureLayout, m_extraUniformsLayout};
        WGPUPipelineLayoutDescriptor pipelineLayoutDesc {};
        pipelineLayoutDesc.bindGroupLayoutCount = 3;
        pipelineLayoutDesc.bindGroupLayouts = layouts;
        m_pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc);

        WGPUBufferDescriptor projectionBufferDesc {};
        projectionBufferDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        projectionBufferDesc.size = sizeof(matrix4x4);
        m_projectionBuffer = wgpuDeviceCreateBuffer(device, &projectionBufferDesc);

        WGPUBindGroupEntry projectionBindEntry {};
        projectionBindEntry.binding = 0;
        projectionBindEntry.buffer = m_projectionBuffer;
        projectionBindEntry.size = sizeof(matrix4x4);

        WGPUBindGroupDescriptor projectionBindDesc {};
        projectionBindDesc.layout = m_projectionLayout;
        projectionBindDesc.entryCount = 1;
        projectionBindDesc.entries = &projectionBindEntry;
        m_projectionBindGroup = wgpuDeviceCreateBindGroup(device, &projectionBindDesc);

        WGPUBufferDescriptor extraUniformsBufferDesc {};
        extraUniformsBufferDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        extraUniformsBufferDesc.size = m_extraUniformsBufferCapacity;
        m_extraUniformsBuffer = wgpuDeviceCreateBuffer(device, &extraUniformsBufferDesc);

        WGPUBindGroupEntry extraUniformsBindEntry {};
        extraUniformsBindEntry.binding = 0;
        extraUniformsBindEntry.buffer = m_extraUniformsBuffer;
        extraUniformsBindEntry.size = kExtraUniformsSlotStride;

        WGPUBindGroupDescriptor extraUniformsBindDesc {};
        extraUniformsBindDesc.layout = m_extraUniformsLayout;
        extraUniformsBindDesc.entryCount = 1;
        extraUniformsBindDesc.entries = &extraUniformsBindEntry;
        m_extraUniformsBindGroup = wgpuDeviceCreateBindGroup(device, &extraUniformsBindDesc);
    }

    Renderer::~Renderer()
    {
        for (auto& [key, entry] : m_pipelines) {
            wgpuRenderPipelineRelease(entry.pipeline);
        }
        for (auto& [key, entry] : m_textureBindGroups) {
            wgpuBindGroupRelease(entry.bindGroup);
        }
        for (auto& [key, sampler] : m_samplers) {
            wgpuSamplerRelease(sampler);
        }

        wgpuBindGroupRelease(m_extraUniformsBindGroup);
        wgpuBufferDestroy(m_extraUniformsBuffer);
        wgpuBufferRelease(m_extraUniformsBuffer);

        wgpuBindGroupRelease(m_projectionBindGroup);
        wgpuBufferDestroy(m_projectionBuffer);
        wgpuBufferRelease(m_projectionBuffer);

        wgpuPipelineLayoutRelease(m_pipelineLayout);
        wgpuBindGroupLayoutRelease(m_extraUniformsLayout);
        wgpuBindGroupLayoutRelease(m_textureLayout);
        wgpuBindGroupLayoutRelease(m_projectionLayout);

        wgpuBufferDestroy(m_indexBuffer);
        wgpuBufferRelease(m_indexBuffer);
        wgpuBufferDestroy(m_vertexBuffer);
        wgpuBufferRelease(m_vertexBuffer);
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

        GpuDevice& gpuDevice = requireDependency<GpuDevice>();
        const matrix4x4 wgslProjectionMatrix = transposed(m_projectionMatrix);
        wgpuQueueWriteBuffer(gpuDevice.getQueue(), m_projectionBuffer, 0, wgslProjectionMatrix.m.data(), sizeof(matrix4x4));

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

        GpuDevice& gpuDevice = requireDependency<GpuDevice>();
        WGPUDevice device = gpuDevice.getDevice();
        WGPUQueue queue = gpuDevice.getQueue();

        wgpuQueueWriteBuffer(queue, m_vertexBuffer, 0, m_vertices.data(), m_currentVertexOffset * sizeof(Vertex));
        wgpuQueueWriteBuffer(queue, m_indexBuffer, 0, m_indices.data(), m_currentIndexOffset * sizeof(uint32_t));
        m_extraUniformsWriteCursor = 0;

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

        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &passDescriptor);

        wgpuRenderPassEncoderSetVertexBuffer(pass, 0, m_vertexBuffer, 0, m_currentVertexOffset * sizeof(Vertex));
        wgpuRenderPassEncoderSetIndexBuffer(pass, m_indexBuffer, WGPUIndexFormat_Uint32, 0, m_currentIndexOffset * sizeof(uint32_t));
        wgpuRenderPassEncoderSetBindGroup(pass, 0, m_projectionBindGroup, 0, nullptr);

        WGPURenderPipeline currentPipeline = nullptr;

        for (const RendererBatch& batch : m_batches) {
            WGPURenderPipeline pipeline = getOrCreatePipeline(batch.shader, batch.blendMode, sampleCount, *colorFormat);
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

            WGPUBindGroup textureBindGroup = getOrCreateTextureBindGroup(batch.texture, batch.textureFilter, batch.textureWrap);
            wgpuRenderPassEncoderSetBindGroup(pass, 1, textureBindGroup, 0, nullptr);

            const uint32_t extraUniformsOffset = writeExtraUniforms(batch);
            wgpuRenderPassEncoderSetBindGroup(pass, 2, m_extraUniformsBindGroup, 1, &extraUniformsOffset);

            wgpuRenderPassEncoderDrawIndexed(pass, static_cast<uint32_t>(batch.indexCount), 1, static_cast<uint32_t>(batch.indexOffset), 0, 0);
        }

        wgpuRenderPassEncoderEnd(pass);
        wgpuRenderPassEncoderRelease(pass);

        WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(queue, 1, &commandBuffer);
        wgpuCommandBufferRelease(commandBuffer);
        wgpuCommandEncoderRelease(encoder);

        m_currentVertexOffset = 0;
        m_currentIndexOffset = 0;
        m_batches.clear();
    }

    WGPURenderPipeline Renderer::getOrCreatePipeline(const Shader& shader, const BlendMode& blendMode, uint32_t sampleCount, WGPUTextureFormat colorFormat)
    {
        const PipelineKey key {shader.impl.get(), blendMode, sampleCount, colorFormat};
        if (const auto it = m_pipelines.find(key); it != m_pipelines.end()) {
            return it->second.pipeline;
        }

        GpuDevice& gpuDevice = requireDependency<GpuDevice>();

        WGPUVertexAttribute attributes[3] = {
            {.nextInChain = nullptr, .format = WGPUVertexFormat_Float32x2, .offset = offsetof(Vertex, position), .shaderLocation = 0},
            {.nextInChain = nullptr, .format = WGPUVertexFormat_Float32x2, .offset = offsetof(Vertex, texCoord), .shaderLocation = 1},
            {.nextInChain = nullptr, .format = WGPUVertexFormat_Float32x4, .offset = offsetof(Vertex, color), .shaderLocation = 2},
        };
        WGPUVertexBufferLayout vertexLayout {};
        vertexLayout.stepMode = WGPUVertexStepMode_Vertex;
        vertexLayout.arrayStride = sizeof(Vertex);
        vertexLayout.attributeCount = 3;
        vertexLayout.attributes = attributes;

        WGPUBlendState blend {};
        blend.color.srcFactor = toWgpu(blendMode.srcColorFactor);
        blend.color.dstFactor = toWgpu(blendMode.dstColorFactor);
        blend.color.operation = toWgpu(blendMode.colorEquation);
        blend.alpha.srcFactor = toWgpu(blendMode.srcAlphaFactor);
        blend.alpha.dstFactor = toWgpu(blendMode.dstAlphaFactor);
        blend.alpha.operation = toWgpu(blendMode.alphaEquation);

        WGPUColorTargetState colorTarget {};
        colorTarget.format = colorFormat;
        colorTarget.blend = &blend;
        colorTarget.writeMask = WGPUColorWriteMask_All;

        WGPUFragmentState fragmentState {};
        fragmentState.module = shader.impl->fragmentModule;
        fragmentState.entryPoint = WGPUStringView {"fs_main", WGPU_STRLEN};
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;

        WGPURenderPipelineDescriptor desc {};
        desc.layout = m_pipelineLayout;
        desc.vertex.module = shader.impl->vertexModule;
        desc.vertex.entryPoint = WGPUStringView {"vs_main", WGPU_STRLEN};
        desc.vertex.bufferCount = 1;
        desc.vertex.buffers = &vertexLayout;
        desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        desc.multisample.count = sampleCount;
        desc.multisample.mask = 0xFFFFFFFF;
        desc.fragment = &fragmentState;

        WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(gpuDevice.getDevice(), &desc);
        if (pipeline == nullptr) {
            error("Renderer: failed to create a render pipeline");
        }

        m_pipelines[key] = PipelineEntry {.shader = shader, .pipeline = pipeline};
        return pipeline;
    }

    WGPUBindGroup Renderer::getOrCreateTextureBindGroup(const Texture& texture, TextureFilter filter, TextureWrap wrap)
    {
        const TextureBindGroupKey key {texture.impl.get(), filter, wrap};
        if (const auto it = m_textureBindGroups.find(key); it != m_textureBindGroups.end()) {
            return it->second.bindGroup;
        }

        WGPUSampler sampler = getOrCreateSampler(filter, wrap);

        WGPUBindGroupEntry entries[2] = {};
        entries[0].binding = 0;
        entries[0].textureView = texture.impl->view;
        entries[1].binding = 1;
        entries[1].sampler = sampler;

        WGPUBindGroupDescriptor desc {};
        desc.layout = m_textureLayout;
        desc.entryCount = 2;
        desc.entries = entries;

        GpuDevice& gpuDevice = requireDependency<GpuDevice>();
        WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(gpuDevice.getDevice(), &desc);

        m_textureBindGroups[key] = TextureBindGroupEntry {.texture = texture, .bindGroup = bindGroup};
        return bindGroup;
    }

    WGPUSampler Renderer::getOrCreateSampler(TextureFilter filter, TextureWrap wrap)
    {
        const auto key = std::make_pair(filter, wrap);
        if (const auto it = m_samplers.find(key); it != m_samplers.end()) {
            return it->second;
        }

        WGPUSamplerDescriptor desc {};
        desc.addressModeU = toWgpu(wrap);
        desc.addressModeV = toWgpu(wrap);
        desc.magFilter = toWgpu(filter);
        desc.minFilter = toWgpu(filter);
        desc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
        desc.maxAnisotropy = 1;

        GpuDevice& gpuDevice = requireDependency<GpuDevice>();
        WGPUSampler sampler = wgpuDeviceCreateSampler(gpuDevice.getDevice(), &desc);

        m_samplers[key] = sampler;
        return sampler;
    }

    uint32_t Renderer::writeExtraUniforms(const RendererBatch& batch)
    {
        if (batch.shader.impl->extraUniformSlots.empty()) {
            return 0;
        }

        if (m_extraUniformsWriteCursor + kExtraUniformsSlotStride > m_extraUniformsBufferCapacity) {
            error("Renderer: extra-uniforms buffer exhausted mid-flush() (an unusually large number of distinct shader/texture batches in one flush) -- reusing slot 0, which may corrupt an earlier batch's uniforms this frame");
            m_extraUniformsWriteCursor = 0;
        }

        std::vector<uint8_t> scratch(batch.shader.impl->extraUniformsByteSize, 0);
        for (const auto& [name, slot] : batch.shader.impl->extraUniformSlots) {
            const auto it = batch.uniforms.find(name);
            if (it == batch.uniforms.end()) {
                continue;
            }

            std::visit([&](const auto& value) {
                using T = std::decay_t<decltype(value)>;
                if (sizeof(T) != slot.byteSize) {
                    warn("setUniform(\"{}\", ...) value type doesn't match the shader's declared @group(2) field type -- ignored", name);
                    return;
                }
                if constexpr (std::is_same_v<T, matrix4x4>) {
                    const matrix4x4 wgslValue = transposed(value);
                    std::memcpy(scratch.data() + slot.byteOffset, wgslValue.m.data(), sizeof(T));
                } else {
                    std::memcpy(scratch.data() + slot.byteOffset, &value, sizeof(T));
                }
            },
                       it->second);
        }

        GpuDevice& gpuDevice = requireDependency<GpuDevice>();
        const uint32_t offset = static_cast<uint32_t>(m_extraUniformsWriteCursor);
        wgpuQueueWriteBuffer(gpuDevice.getQueue(), m_extraUniformsBuffer, offset, scratch.data(), scratch.size());

        m_extraUniformsWriteCursor += kExtraUniformsSlotStride;
        return offset;
    }

    void Renderer::appendVertex(const Vertex& vertex)
    {
        if (m_currentVertexOffset >= m_vertices.size()) {
            m_vertices.resize(m_vertices.size() * 2);
        }

        m_vertices[m_currentVertexOffset++] = vertex;

        if (m_vertices.size() > m_uploadedVertexCapacity) {
            wgpuBufferDestroy(m_vertexBuffer);
            wgpuBufferRelease(m_vertexBuffer);

            GpuDevice& gpuDevice = requireDependency<GpuDevice>();
            WGPUBufferDescriptor desc {};
            desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
            desc.size = m_vertices.size() * sizeof(Vertex);
            m_vertexBuffer = wgpuDeviceCreateBuffer(gpuDevice.getDevice(), &desc);

            m_uploadedVertexCapacity = m_vertices.size();
        }
    }

    void Renderer::appendIndex(uint32_t index)
    {
        if (m_currentIndexOffset >= m_indices.size()) {
            m_indices.resize(m_indices.size() * 2);
        }

        m_indices[m_currentIndexOffset++] = index;

        if (m_indices.size() > m_uploadedIndexCapacity) {
            wgpuBufferDestroy(m_indexBuffer);
            wgpuBufferRelease(m_indexBuffer);

            GpuDevice& gpuDevice = requireDependency<GpuDevice>();
            WGPUBufferDescriptor desc {};
            desc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
            desc.size = m_indices.size() * sizeof(uint32_t);
            m_indexBuffer = wgpuDeviceCreateBuffer(gpuDevice.getDevice(), &desc);

            m_uploadedIndexCapacity = m_indices.size();
        }
    }

    void Renderer::flushIfNearCapacity()
    {
        const size_t maxVertexCount = m_vertices.size();
        const size_t maxIndexCount = m_indices.size();
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

    void Renderer::finish(const Writer& writer, const BlendMode& blendMode, const std::optional<rect2f>& clipRect, TextureFilter textureFilter, TextureWrap textureWrap, const Texture& texture, const Shader& shader, const std::unordered_map<std::string, UniformValue>& uniforms)
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
