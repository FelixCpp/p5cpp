#include <p5cpp/p5cpp.hpp>
#include <p5cpp/graphics/default_shaders.hpp>
#include <p5cpp/graphics/shader_impl.hpp>
#include <p5cpp/graphics/gpu_device.hpp>

#include <webgpu/webgpu.h>

#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

namespace p5
{
    namespace
    {
        struct UniformTypeLayout
        {
            uint32_t align;
            uint32_t size;
        };

        UniformTypeLayout layoutOf(UniformType type)
        {
            switch (type) {
                case UniformType::f32: return {4, 4};
                case UniformType::vec2f: return {8, 8};
                case UniformType::vec3f: return {16, 12};
                case UniformType::vec4f: return {16, 16};
                case UniformType::mat4x4f: return {16, 64};
            }
            return {4, 4};
        }

        uint32_t alignUp(uint32_t value, uint32_t alignment)
        {
            return (value + alignment - 1) / alignment * alignment;
        }

        std::pair<std::unordered_map<std::string, ShaderUniformSlot>, uint32_t> computeUniformLayout(std::initializer_list<UniformDecl> decls)
        {
            std::unordered_map<std::string, ShaderUniformSlot> slots;
            uint32_t byteSize = 0;
            for (const UniformDecl& decl : decls) {
                const UniformTypeLayout layout = layoutOf(decl.type);
                const uint32_t offset = alignUp(byteSize, layout.align);
                slots[std::string(decl.name)] = ShaderUniformSlot {.byteOffset = offset, .byteSize = layout.size};
                byteSize = offset + layout.size;
            }
            return {slots, byteSize};
        }

        WGPUShaderModule createShaderModule(WGPUDevice device, std::string_view source, const char* label)
        {
            WGPUShaderSourceWGSL wgslSource {};
            wgslSource.chain.sType = WGPUSType_ShaderSourceWGSL;
            wgslSource.code = WGPUStringView {source.data(), source.size()};

            WGPUShaderModuleDescriptor desc {};
            desc.nextInChain = &wgslSource.chain;
            desc.label = WGPUStringView {label, WGPU_STRLEN};

            return wgpuDeviceCreateShaderModule(device, &desc);
        }

        // The body passed to loadShaderFromMemory(effectSource) only defines a WGSL `effect()`
        // function (plus, optionally, its own @group(2) uniforms -- see p5cpp.hpp's doc comment
        // for the full convention); this header/footer is concatenated around it to form a
        // complete fragment shader.
        inline static constexpr std::string_view effectFragmentHeaderSource = R"(
            @group(1) @binding(0) var u_Texture: texture_2d<f32>;
            @group(1) @binding(1) var u_Sampler: sampler;
        )";

        inline static constexpr std::string_view effectFragmentFooterSource = R"(
            @fragment
            fn fs_main(
                @location(0) v_TexCoord: vec2f,
                @location(1) v_Color: vec4f,
                @builtin(position) v_FragCoord: vec4f,
            ) -> @location(0) vec4f {
                return effect(v_Color, u_Texture, u_Sampler, v_TexCoord, v_FragCoord.xy);
            }
        )";
    } // namespace

    ShaderImpl::~ShaderImpl()
    {
        if (fragmentModule != nullptr) {
            wgpuShaderModuleRelease(fragmentModule);
        }
        if (vertexModule != nullptr) {
            wgpuShaderModuleRelease(vertexModule);
        }
    }

    bool Shader::isValid() const
    {
        return impl != nullptr;
    }

    std::optional<Shader> loadShaderFromMemory(std::string_view vertexShaderSource, std::string_view fragmentShaderSource, std::initializer_list<UniformDecl> extraUniforms)
    {
        GpuDevice& gpuDevice = requireDependency<GpuDevice>();
        WGPUDevice device = gpuDevice.getDevice();

        WGPUShaderModule vertexModule = createShaderModule(device, vertexShaderSource, "p5cpp vertex shader");
        if (vertexModule == nullptr) {
            return std::nullopt;
        }

        WGPUShaderModule fragmentModule = createShaderModule(device, fragmentShaderSource, "p5cpp fragment shader");
        if (fragmentModule == nullptr) {
            wgpuShaderModuleRelease(vertexModule);
            return std::nullopt;
        }

        auto impl = std::make_shared<ShaderImpl>();
        impl->vertexModule = vertexModule;
        impl->fragmentModule = fragmentModule;
        std::tie(impl->extraUniformSlots, impl->extraUniformsByteSize) = computeUniformLayout(extraUniforms);

        return Shader {.impl = std::move(impl)};
    }

    std::optional<Shader> loadShaderFromMemory(std::string_view effectSource, std::initializer_list<UniformDecl> extraUniforms)
    {
        std::string fragmentSource;
        fragmentSource.reserve(effectFragmentHeaderSource.size() + effectSource.size() + effectFragmentFooterSource.size());
        fragmentSource += effectFragmentHeaderSource;
        fragmentSource += effectSource;
        fragmentSource += effectFragmentFooterSource;

        return loadShaderFromMemory(detail::defaultVertexShaderSource, fragmentSource, extraUniforms);
    }

    std::optional<Shader> loadShaderFromFile(const std::filesystem::path& effectFilepath, std::initializer_list<UniformDecl> extraUniforms)
    {
        std::ifstream file(effectFilepath, std::ios::binary);
        if (not file) {
            error("loadShaderFromFile() failed to open \"{}\"", effectFilepath.string());
            return std::nullopt;
        }

        std::ostringstream contents;
        contents << file.rdbuf();
        return loadShaderFromMemory(contents.str(), extraUniforms);
    }
} // namespace p5
