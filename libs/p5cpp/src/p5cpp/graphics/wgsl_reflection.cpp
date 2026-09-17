#include <p5cpp/graphics/wgsl_reflection.hpp>
#include <p5cpp/graphics/wgsl_conventions.hpp>
#include <p5cpp/p5cpp.hpp>

#include <algorithm>
#include <optional>
#include <regex>

namespace p5
{
    std::vector<WgslVarDecl> scanWgslVarDecls(std::string_view source, uint32_t group, std::string_view kind)
    {
        std::vector<WgslVarDecl> decls;
        const std::string sourceStr(source);

        if (kind == "storage") {
            const std::regex pattern(
                R"(@group\()" + std::to_string(group) + R"(\)\s*@binding\((\d+)\)\s*var<storage\s*,\s*(read_write|read)\s*>\s+(\w+)\s*:\s*([A-Za-z0-9_<>]+))"
            );
            for (auto it = std::sregex_iterator(sourceStr.begin(), sourceStr.end(), pattern); it != std::sregex_iterator(); ++it) {
                const std::smatch& match = *it;
                decls.push_back(WgslVarDecl {
                    .binding = static_cast<uint32_t>(std::stoul(match[1].str())),
                    .storageMode = match[2].str(),
                    .name = match[3].str(),
                    .typeName = match[4].str(),
                });
            }
        } else {
            const std::regex pattern(
                R"(@group\()" + std::to_string(group) + R"(\)\s*@binding\((\d+)\)\s*var<uniform>\s+(\w+)\s*:\s*([A-Za-z0-9_<>]+))"
            );
            for (auto it = std::sregex_iterator(sourceStr.begin(), sourceStr.end(), pattern); it != std::sregex_iterator(); ++it) {
                const std::smatch& match = *it;
                decls.push_back(WgslVarDecl {
                    .binding = static_cast<uint32_t>(std::stoul(match[1].str())),
                    .storageMode = "",
                    .name = match[2].str(),
                    .typeName = match[3].str(),
                });
            }
        }

        return decls;
    }

    namespace
    {
        struct WgslTypeLayout
        {
            uint32_t align;
            uint32_t size;
        };

        std::optional<WgslTypeLayout> wgslTypeLayoutOf(const std::string& typeName)
        {
            if (typeName == "f32") return WgslTypeLayout {4, 4};
            if (typeName == "vec2f" or typeName == "vec2<f32>") return WgslTypeLayout {8, 8};
            if (typeName == "vec3f" or typeName == "vec3<f32>") return WgslTypeLayout {16, 12};
            if (typeName == "vec4f" or typeName == "vec4<f32>") return WgslTypeLayout {16, 16};
            if (typeName == "mat4x4f" or typeName == "mat4x4<f32>") return WgslTypeLayout {16, 64};
            return std::nullopt;
        }

        uint32_t alignUp(uint32_t value, uint32_t alignment)
        {
            return (value + alignment - 1) / alignment * alignment;
        }
    } // namespace

    std::pair<std::unordered_map<std::string, ShaderUniformSlot>, uint32_t> parseExtraUniformLayout(std::string_view fragmentShaderSource)
    {
        std::unordered_map<std::string, ShaderUniformSlot> slots;
        uint32_t byteSize = 0;

        const std::vector<WgslVarDecl> decls = scanWgslVarDecls(fragmentShaderSource, wgsl::kExtraUniformsBindGroup, "uniform");
        const auto declIt = std::find_if(decls.begin(), decls.end(), [](const WgslVarDecl& d) {
            return d.binding == 0;
        });

        if (declIt == decls.end()) {
            return {slots, byteSize};
        }

        const std::string& structName = declIt->typeName;

        const std::string source(fragmentShaderSource);
        const std::regex structPattern("struct\\s+" + structName + R"(\s*\{([^}]*)\})");
        std::smatch structMatch;
        if (not std::regex_search(source, structMatch, structPattern)) {
            error("Shader: found @group({}) @binding(0) var<uniform> of type '{}' but no matching struct declaration", wgsl::kExtraUniformsBindGroup, structName);
            return {slots, byteSize};
        }

        const std::string body = structMatch[1].str();
        static const std::regex fieldPattern(R"(([A-Za-z_]\w*)\s*:\s*([A-Za-z0-9_<>]+))");
        for (auto it = std::sregex_iterator(body.begin(), body.end(), fieldPattern); it != std::sregex_iterator(); ++it) {
            const std::string fieldName = (*it)[1].str();
            const std::string fieldType = (*it)[2].str();

            const std::optional<WgslTypeLayout> layout = wgslTypeLayoutOf(fieldType);
            if (not layout.has_value()) {
                error("Shader: extra-uniform field '{}' has unsupported type '{}' (supported: f32, vec2f, vec3f, vec4f, mat4x4f)", fieldName, fieldType);
                continue;
            }

            const uint32_t offset = alignUp(byteSize, layout->align);
            slots[fieldName] = ShaderUniformSlot {.byteOffset = offset, .byteSize = layout->size};
            byteSize = offset + layout->size;
        }

        return {slots, byteSize};
    }

    std::unordered_map<std::string, ComputeBindingSlot> parseComputeBindingLayout(std::string_view computeShaderSource)
    {
        std::unordered_map<std::string, ComputeBindingSlot> slots;

        const std::vector<WgslVarDecl> decls = scanWgslVarDecls(computeShaderSource, wgsl::kComputeStorageBindGroup, "storage");
        for (const WgslVarDecl& decl : decls) {
            slots[decl.name] = ComputeBindingSlot {.binding = decl.binding, .readOnly = decl.storageMode == "read"};
        }

        return slots;
    }
} // namespace p5
