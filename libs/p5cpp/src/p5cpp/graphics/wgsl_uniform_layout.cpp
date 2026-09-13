#include <p5cpp/graphics/wgsl_uniform_layout.hpp>
#include <p5cpp/p5cpp.hpp>

#include <optional>
#include <regex>
#include <string>

namespace p5
{
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

        static const std::regex bindingPattern(R"(@group\(2\)\s*@binding\(0\)\s*var<uniform>\s+\w+\s*:\s*(\w+))");
        const std::string source(fragmentShaderSource);
        std::smatch bindingMatch;
        if (not std::regex_search(source, bindingMatch, bindingPattern)) {
            return {slots, byteSize};
        }
        const std::string structName = bindingMatch[1].str();

        const std::regex structPattern("struct\\s+" + structName + R"(\s*\{([^}]*)\})");
        std::smatch structMatch;
        if (not std::regex_search(source, structMatch, structPattern)) {
            error("Shader: found @group(2) @binding(0) var<uniform> of type '{}' but no matching struct declaration", structName);
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
} // namespace p5
