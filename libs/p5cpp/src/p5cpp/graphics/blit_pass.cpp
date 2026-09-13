#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/graphics_impl.hpp>
#include <p5cpp/graphics/texture_impl.hpp>
#include <p5cpp/graphics/gpu_device.hpp>

#include <webgpu/webgpu.h>

#include <map>
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

        struct BlitResources
        {
            WGPUBindGroupLayout bindGroupLayout;
            WGPUPipelineLayout pipelineLayout;
            WGPUShaderModule vertexModule;
            WGPUShaderModule fragmentModule;
            WGPUSampler sampler;
            std::map<WGPUTextureFormat, WGPURenderPipeline> pipelines;
        };

        BlitResources& getBlitResources(WGPUDevice device)
        {
            static std::map<WGPUDevice, BlitResources> perDeviceResources;
            if (const auto it = perDeviceResources.find(device); it != perDeviceResources.end()) {
                return it->second;
            }

            BlitResources resources = [device] {
                BlitResources r {};

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
                r.bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &layoutDesc);

                WGPUPipelineLayoutDescriptor pipelineLayoutDesc {};
                pipelineLayoutDesc.bindGroupLayoutCount = 1;
                pipelineLayoutDesc.bindGroupLayouts = &r.bindGroupLayout;
                r.pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc);

                r.vertexModule = createBlitShaderModule(device, blitVertexShaderSource);
                r.fragmentModule = createBlitShaderModule(device, blitFragmentShaderSource);

                WGPUSamplerDescriptor samplerDesc {};
                samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
                samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
                samplerDesc.magFilter = WGPUFilterMode_Linear;
                samplerDesc.minFilter = WGPUFilterMode_Linear;
                samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
                samplerDesc.maxAnisotropy = 1;
                r.sampler = wgpuDeviceCreateSampler(device, &samplerDesc);

                return r;
            }();

            return perDeviceResources.emplace(device, std::move(resources)).first->second;
        }

        WGPURenderPipeline getOrCreateBlitPipeline(WGPUDevice device, WGPUTextureFormat targetFormat)
        {
            BlitResources& resources = getBlitResources(device);

            if (const auto it = resources.pipelines.find(targetFormat); it != resources.pipelines.end()) {
                return it->second;
            }

            WGPUColorTargetState colorTarget {};
            colorTarget.format = targetFormat;
            colorTarget.writeMask = WGPUColorWriteMask_All;

            WGPUFragmentState fragmentState {};
            fragmentState.module = resources.fragmentModule;
            fragmentState.entryPoint = WGPUStringView {"fs_main", WGPU_STRLEN};
            fragmentState.targetCount = 1;
            fragmentState.targets = &colorTarget;

            WGPURenderPipelineDescriptor desc {};
            desc.layout = resources.pipelineLayout;
            desc.vertex.module = resources.vertexModule;
            desc.vertex.entryPoint = WGPUStringView {"vs_main", WGPU_STRLEN};
            desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
            desc.multisample.count = 1;
            desc.multisample.mask = 0xFFFFFFFF;
            desc.fragment = &fragmentState;

            WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(device, &desc);
            resources.pipelines[targetFormat] = pipeline;
            return pipeline;
        }
    } // namespace

    void blitGraphicsToScreen(const Graphics& graphics, [[maybe_unused]] uint32_t screenWidth, [[maybe_unused]] uint32_t screenHeight)
    {
        GpuDevice& gpuDevice = requireDependency<GpuDevice>();
        WGPUDevice device = gpuDevice.getDevice();

        WGPUTexture surfaceTexture = gpuDevice.acquireNextSurfaceTexture();
        if (surfaceTexture == nullptr) {
            return;
        }

        WGPURenderPipeline pipeline = getOrCreateBlitPipeline(device, gpuDevice.getSurfaceFormat());
        BlitResources& resources = getBlitResources(device);

        WGPUBindGroupEntry bindEntries[2] = {};
        bindEntries[0].binding = 0;
        bindEntries[0].textureView = graphics.colorTexture.impl->view;
        bindEntries[1].binding = 1;
        bindEntries[1].sampler = resources.sampler;

        WGPUBindGroupDescriptor bindGroupDesc {};
        bindGroupDesc.layout = resources.bindGroupLayout;
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

        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &passDescriptor);

        wgpuRenderPassEncoderSetPipeline(pass, pipeline);
        wgpuRenderPassEncoderSetBindGroup(pass, 0, bindGroup, 0, nullptr);
        wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0);

        wgpuRenderPassEncoderEnd(pass);
        wgpuRenderPassEncoderRelease(pass);

        WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(gpuDevice.getQueue(), 1, &commandBuffer);
        wgpuCommandBufferRelease(commandBuffer);
        wgpuCommandEncoderRelease(encoder);

        wgpuTextureViewRelease(surfaceView);
        wgpuBindGroupRelease(bindGroup);
    }
} // namespace p5
