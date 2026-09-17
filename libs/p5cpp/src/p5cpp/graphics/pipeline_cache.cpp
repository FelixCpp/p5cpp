#include <p5cpp/graphics/pipeline_cache.hpp>
#include <p5cpp/graphics/shader_impl.hpp>
#include <p5cpp/graphics/gpu_device.hpp>
#include <p5cpp/graphics/wgsl_conventions.hpp>

#include <cstddef>
#include <tuple>

namespace p5
{
    namespace
    {
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
                    error("PipelineCache: invalid BlendMode::Factor, falling back to one");
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
                    error("PipelineCache: invalid BlendMode::Equation, falling back to add");
                    return WGPUBlendOperation_Add;
            }
        }
    } // namespace

    bool PipelineCache::Key::operator<(const Key& other) const
    {
        return std::tie(shader, blendMode.srcColorFactor, blendMode.dstColorFactor, blendMode.colorEquation, blendMode.srcAlphaFactor, blendMode.dstAlphaFactor, blendMode.alphaEquation, sampleCount, colorFormat) < std::tie(other.shader, other.blendMode.srcColorFactor, other.blendMode.dstColorFactor, other.blendMode.colorEquation, other.blendMode.srcAlphaFactor, other.blendMode.dstAlphaFactor, other.blendMode.alphaEquation, other.sampleCount, other.colorFormat);
    }

    PipelineCache::PipelineCache(GpuDevice& gpuDevice, WGPUPipelineLayout pipelineLayout)
        : m_gpuDevice(gpuDevice),
          m_pipelineLayout(pipelineLayout)
    {
    }

    PipelineCache::~PipelineCache()
    {
        for (auto& [key, entry] : m_pipelines) {
            wgpuRenderPipelineRelease(entry.pipeline);
        }
    }

    WGPURenderPipeline PipelineCache::getOrCreate(const Shader& shader, const BlendMode& blendMode, uint32_t sampleCount, WGPUTextureFormat colorFormat)
    {
        const Key key {shader.impl.get(), blendMode, sampleCount, colorFormat};
        if (const auto it = m_pipelines.find(key); it != m_pipelines.end()) {
            return it->second.pipeline;
        }

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
        fragmentState.entryPoint = WGPUStringView {wgsl::kFragmentEntryPoint, WGPU_STRLEN};
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;

        WGPURenderPipelineDescriptor desc {};
        desc.layout = m_pipelineLayout;
        desc.vertex.module = shader.impl->vertexModule;
        desc.vertex.entryPoint = WGPUStringView {wgsl::kVertexEntryPoint, WGPU_STRLEN};
        desc.vertex.bufferCount = 1;
        desc.vertex.buffers = &vertexLayout;
        desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        desc.multisample.count = sampleCount;
        desc.multisample.mask = 0xFFFFFFFF;
        desc.fragment = &fragmentState;

        WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(m_gpuDevice.getDevice(), &desc);
        if (pipeline == nullptr) {
            error("PipelineCache: failed to create a render pipeline");
        }

        m_pipelines[key] = Entry {.shader = shader, .pipeline = pipeline};
        return pipeline;
    }

    void PipelineCache::evictUnused()
    {
        for (auto it = m_pipelines.begin(); it != m_pipelines.end();) {
            if (it->second.shader.impl.use_count() == 1) {
                wgpuRenderPipelineRelease(it->second.pipeline);
                it = m_pipelines.erase(it);
            } else {
                ++it;
            }
        }
    }
} // namespace p5
