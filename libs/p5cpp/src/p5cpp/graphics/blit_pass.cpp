#include <p5cpp/graphics/blit_pass.hpp>
#include <p5cpp/graphics/graphics_impl.hpp>
#include <p5cpp/graphics/texture_impl.hpp>
#include <p5cpp/graphics/gpu_device.hpp>
#include <p5cpp/graphics/gpu_command.hpp>
#include <p5cpp/graphics/wgsl_conventions.hpp>
#include <p5cpp/graphics/graphics_plugin.hpp>

#include <webgpu/webgpu.h>

#include <string_view>

namespace p5
{
    namespace
    {
        inline static constexpr std::string_view blitVertexShaderSource = R"(
            struct VertexOutput {
                @builtin(position) position: vec4f,
                @location(0) uv: vec2f,
            };

            @vertex
            fn vs_main(@builtin(vertex_index) vertexIndex: u32) -> VertexOutput {
                var positions = array<vec2f, 3>(
                    vec2f(-1.0, -1.0),
                    vec2f(3.0, -1.0),
                    vec2f(-1.0, 3.0),
                );
                let pos = positions[vertexIndex];

                var out: VertexOutput;
                out.position = vec4f(pos, 0.0, 1.0);
                out.uv = vec2f((pos.x + 1.0) * 0.5, 1.0 - (pos.y + 1.0) * 0.5);
                return out;
            }
        )";

        inline static constexpr std::string_view blitFragmentShaderSource = R"(
            @group(0) @binding(0) var u_Texture: texture_2d<f32>;
            @group(0) @binding(1) var u_Sampler: sampler;

            @fragment
            fn fs_main(@location(0) uv: vec2f) -> @location(0) vec4f {
                return textureSample(u_Texture, u_Sampler, uv);
            }
        )";

        WGPUShaderModule createBlitShaderModule(WGPUDevice device, std::string_view source)
        {
            WGPUShaderSourceWGSL wgslSource {};
            wgslSource.chain.sType = WGPUSType_ShaderSourceWGSL;
            wgslSource.code = WGPUStringView {source.data(), source.size()};

            WGPUShaderModuleDescriptor desc {};
            desc.nextInChain = &wgslSource.chain;
            return wgpuDeviceCreateShaderModule(device, &desc);
        }
    } // namespace

    BlitPass::BlitPass(GpuDevice& gpuDevice)
        : m_gpuDevice(gpuDevice)
    {
        WGPUDevice device = gpuDevice.getDevice();

        WGPUBindGroupLayoutEntry entries[2] = {};
        entries[0].binding = 0;
        entries[0].visibility = WGPUShaderStage_Fragment;
        entries[0].texture.sampleType = WGPUTextureSampleType_Float;
        entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
        entries[1].binding = 1;
        entries[1].visibility = WGPUShaderStage_Fragment;
        entries[1].sampler.type = WGPUSamplerBindingType_Filtering;

        WGPUBindGroupLayoutDescriptor layoutDesc {};
        layoutDesc.entryCount = 2;
        layoutDesc.entries = entries;
        m_bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &layoutDesc);

        WGPUPipelineLayoutDescriptor pipelineLayoutDesc {};
        pipelineLayoutDesc.bindGroupLayoutCount = 1;
        pipelineLayoutDesc.bindGroupLayouts = &m_bindGroupLayout;
        m_pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc);

        m_vertexModule = createBlitShaderModule(device, blitVertexShaderSource);
        m_fragmentModule = createBlitShaderModule(device, blitFragmentShaderSource);

        WGPUSamplerDescriptor samplerDesc {};
        samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
        samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
        samplerDesc.magFilter = WGPUFilterMode_Linear;
        samplerDesc.minFilter = WGPUFilterMode_Linear;
        samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
        samplerDesc.maxAnisotropy = 1;
        m_sampler = wgpuDeviceCreateSampler(device, &samplerDesc);
    }

    BlitPass::~BlitPass()
    {
        for (auto& [format, pipeline] : m_pipelines) {
            wgpuRenderPipelineRelease(pipeline);
        }

        wgpuSamplerRelease(m_sampler);
        wgpuShaderModuleRelease(m_fragmentModule);
        wgpuShaderModuleRelease(m_vertexModule);
        wgpuPipelineLayoutRelease(m_pipelineLayout);
        wgpuBindGroupLayoutRelease(m_bindGroupLayout);
    }

    WGPURenderPipeline BlitPass::getOrCreatePipeline(WGPUTextureFormat targetFormat)
    {
        if (const auto it = m_pipelines.find(targetFormat); it != m_pipelines.end()) {
            return it->second;
        }

        WGPUColorTargetState colorTarget {};
        colorTarget.format = targetFormat;
        colorTarget.writeMask = WGPUColorWriteMask_All;

        WGPUFragmentState fragmentState {};
        fragmentState.module = m_fragmentModule;
        fragmentState.entryPoint = WGPUStringView {wgsl::kFragmentEntryPoint, WGPU_STRLEN};
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;

        WGPURenderPipelineDescriptor desc {};
        desc.layout = m_pipelineLayout;
        desc.vertex.module = m_vertexModule;
        desc.vertex.entryPoint = WGPUStringView {wgsl::kVertexEntryPoint, WGPU_STRLEN};
        desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        desc.multisample.count = 1;
        desc.multisample.mask = 0xFFFFFFFF;
        desc.fragment = &fragmentState;

        WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(m_gpuDevice.getDevice(), &desc);
        m_pipelines[targetFormat] = pipeline;
        return pipeline;
    }

    void BlitPass::blit(const Graphics& graphics)
    {
        WGPUDevice device = m_gpuDevice.getDevice();

        WGPUTexture surfaceTexture = m_gpuDevice.acquireNextSurfaceTexture();
        if (surfaceTexture == nullptr) {
            return;
        }

        WGPURenderPipeline pipeline = getOrCreatePipeline(m_gpuDevice.getSurfaceFormat());

        WGPUBindGroupEntry bindEntries[2] = {};
        bindEntries[0].binding = 0;
        bindEntries[0].textureView = graphics.colorTexture.impl->view;
        bindEntries[1].binding = 1;
        bindEntries[1].sampler = m_sampler;

        WGPUBindGroupDescriptor bindGroupDesc {};
        bindGroupDesc.layout = m_bindGroupLayout;
        bindGroupDesc.entryCount = 2;
        bindGroupDesc.entries = bindEntries;
        WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);

        WGPUTextureView surfaceView = wgpuTextureCreateView(surfaceTexture, nullptr);

        WGPURenderPassColorAttachment colorAttachment {};
        colorAttachment.view = surfaceView;
        colorAttachment.loadOp = WGPULoadOp_Load;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

        WGPURenderPassDescriptor passDescriptor {};
        passDescriptor.colorAttachmentCount = 1;
        passDescriptor.colorAttachments = &colorAttachment;

        GpuCommandScope commands(device);
        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(commands.encoder(), &passDescriptor);

        wgpuRenderPassEncoderSetPipeline(pass, pipeline);
        wgpuRenderPassEncoderSetBindGroup(pass, 0, bindGroup, 0, nullptr);
        wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0);

        wgpuRenderPassEncoderEnd(pass);
        wgpuRenderPassEncoderRelease(pass);

        commands.submit(m_gpuDevice.getQueue());

        wgpuTextureViewRelease(surfaceView);
        wgpuBindGroupRelease(bindGroup);
    }
} // namespace p5
