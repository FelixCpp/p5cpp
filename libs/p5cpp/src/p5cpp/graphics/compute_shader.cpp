#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/compute_shader_impl.hpp>
#include <p5cpp/graphics/wgsl_reflection.hpp>
#include <p5cpp/graphics/wgsl_conventions.hpp>
#include <p5cpp/graphics/storage_buffer_impl.hpp>
#include <p5cpp/graphics/gpu_device.hpp>
#include <p5cpp/graphics/gpu_command.hpp>

#include <webgpu/webgpu.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

namespace p5
{
    ComputeShaderImpl::~ComputeShaderImpl()
    {
        for (auto& [key, entry] : bindGroupCache) {
            wgpuBindGroupRelease(entry.bindGroup);
        }
        if (pipeline != nullptr) {
            wgpuComputePipelineRelease(pipeline);
        }
        if (pipelineLayout != nullptr) {
            wgpuPipelineLayoutRelease(pipelineLayout);
        }
        if (bindGroupLayout != nullptr) {
            wgpuBindGroupLayoutRelease(bindGroupLayout);
        }
        if (module != nullptr) {
            wgpuShaderModuleRelease(module);
        }
    }

    bool ComputeShader::isValid() const
    {
        return impl != nullptr;
    }

    std::optional<ComputeShader> loadComputeShaderFromMemory(std::string_view source)
    {
        GpuDevice& gpuDevice = requireDependency<GpuDevice>();
        WGPUDevice device = gpuDevice.getDevice();

        WGPUShaderSourceWGSL wgslSource {};
        wgslSource.chain.sType = WGPUSType_ShaderSourceWGSL;
        wgslSource.code = WGPUStringView {source.data(), source.size()};

        WGPUShaderModuleDescriptor moduleDesc {};
        moduleDesc.nextInChain = &wgslSource.chain;
        moduleDesc.label = WGPUStringView {"p5cpp compute shader", WGPU_STRLEN};
        WGPUShaderModule module = wgpuDeviceCreateShaderModule(device, &moduleDesc);
        if (module == nullptr) {
            error("loadComputeShaderFromMemory() failed to compile the WGSL source");
            return std::nullopt;
        }

        std::unordered_map<std::string, ComputeBindingSlot> bindingSlots = parseComputeBindingLayout(source);

        std::vector<WGPUBindGroupLayoutEntry> entries;
        entries.reserve(bindingSlots.size());
        for (const auto& [name, slot] : bindingSlots) {
            WGPUBindGroupLayoutEntry entry {};
            entry.binding = slot.binding;
            entry.visibility = WGPUShaderStage_Compute;
            entry.buffer.type = slot.readOnly ? WGPUBufferBindingType_ReadOnlyStorage : WGPUBufferBindingType_Storage;
            entries.push_back(entry);
        }

        WGPUBindGroupLayoutDescriptor layoutDesc {};
        layoutDesc.entryCount = entries.size();
        layoutDesc.entries = entries.data();
        WGPUBindGroupLayout bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &layoutDesc);

        WGPUPipelineLayoutDescriptor pipelineLayoutDesc {};
        pipelineLayoutDesc.bindGroupLayoutCount = 1;
        pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout;
        WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc);

        WGPUComputePipelineDescriptor pipelineDesc {};
        pipelineDesc.layout = pipelineLayout;
        pipelineDesc.compute.module = module;
        pipelineDesc.compute.entryPoint = WGPUStringView {wgsl::kComputeEntryPoint, WGPU_STRLEN};
        WGPUComputePipeline pipeline = wgpuDeviceCreateComputePipeline(device, &pipelineDesc);
        if (pipeline == nullptr) {
            error("loadComputeShaderFromMemory() failed to create the compute pipeline (entry point must be a function named \"main\")");
            wgpuPipelineLayoutRelease(pipelineLayout);
            wgpuBindGroupLayoutRelease(bindGroupLayout);
            wgpuShaderModuleRelease(module);
            return std::nullopt;
        }

        auto impl = std::make_shared<ComputeShaderImpl>();
        impl->module = module;
        impl->bindGroupLayout = bindGroupLayout;
        impl->pipelineLayout = pipelineLayout;
        impl->pipeline = pipeline;
        impl->bindingSlots = std::move(bindingSlots);

        return ComputeShader {.impl = std::move(impl)};
    }

    std::optional<ComputeShader> loadComputeShaderFromFile(const std::filesystem::path& filepath)
    {
        std::ifstream file(filepath, std::ios::binary);
        if (not file) {
            error("loadComputeShaderFromFile() failed to open \"{}\"", filepath.string());
            return std::nullopt;
        }

        std::ostringstream contents;
        contents << file.rdbuf();
        return loadComputeShaderFromMemory(contents.str());
    }

    void dispatchCompute(const ComputeShader& shader, std::initializer_list<ComputeBinding> bindings, uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ)
    {
        if (not shader.isValid()) {
            error("dispatchCompute() called with an invalid ComputeShader");
            return;
        }

        GpuDevice& gpuDevice = requireDependency<GpuDevice>();

        std::vector<std::pair<uint32_t, StorageBuffer>> resolved;
        resolved.reserve(bindings.size());
        for (const auto& [name, buffer] : bindings) {
            const auto it = shader.impl->bindingSlots.find(std::string(name));
            if (it == shader.impl->bindingSlots.end()) {
                error("dispatchCompute() shader has no storage buffer binding named '{}'", name);
                continue;
            }
            if (not buffer.isValid()) {
                error("dispatchCompute() binding '{}' is an invalid StorageBuffer", name);
                continue;
            }

            resolved.emplace_back(it->second.binding, buffer);
        }
        std::sort(resolved.begin(), resolved.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

        std::vector<StorageBufferImpl*> cacheKey;
        cacheKey.reserve(resolved.size());
        for (const auto& [bindingIndex, buffer] : resolved) {
            cacheKey.push_back(buffer.impl.get());
        }

        WGPUBindGroup bindGroup;
        if (const auto it = shader.impl->bindGroupCache.find(cacheKey); it != shader.impl->bindGroupCache.end()) {
            bindGroup = it->second.bindGroup;
        } else {
            std::vector<WGPUBindGroupEntry> bindGroupEntries;
            std::vector<StorageBuffer> keptAlive;
            bindGroupEntries.reserve(resolved.size());
            keptAlive.reserve(resolved.size());
            for (const auto& [bindingIndex, buffer] : resolved) {
                WGPUBindGroupEntry entry {};
                entry.binding = bindingIndex;
                entry.buffer = buffer.impl->buffer;
                entry.size = buffer.byteSize;
                bindGroupEntries.push_back(entry);
                keptAlive.push_back(buffer);
            }

            WGPUBindGroupDescriptor bindGroupDesc {};
            bindGroupDesc.layout = shader.impl->bindGroupLayout;
            bindGroupDesc.entryCount = bindGroupEntries.size();
            bindGroupDesc.entries = bindGroupEntries.data();
            bindGroup = wgpuDeviceCreateBindGroup(gpuDevice.getDevice(), &bindGroupDesc);

            shader.impl->bindGroupCache[cacheKey] = ComputeBindGroupEntry {.buffers = std::move(keptAlive), .bindGroup = bindGroup};
        }

        GpuCommandScope commands(gpuDevice.getDevice());
        WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(commands.encoder(), nullptr);
        wgpuComputePassEncoderSetPipeline(pass, shader.impl->pipeline);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bindGroup, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, groupsX, groupsY, groupsZ);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);

        commands.submit(gpuDevice.getQueue());
    }
} // namespace p5
