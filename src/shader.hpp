#pragma once

#include "p5.hpp"

#include <string_view>

namespace p5
{
    std::unique_ptr<Shader> loadShader(std::string_view vertexSource, std::string_view fragmentSource);
    std::unique_ptr<Shader> createDefaultShader();
    std::unique_ptr<Shader> createTextShader();
} // namespace p5
