#pragma once

#include <p5cpp/graphics/shader.hpp>

namespace p5cpp
{
    std::unique_ptr<Shader> createPrimitiveShader();
    std::unique_ptr<Shader> createTextShader();
} // namespace p5cpp
