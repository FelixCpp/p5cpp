#pragma once

#include <p5cpp/graphics/shader.hpp>

namespace p5cpp
{
    std::unique_ptr<ShaderImpl> createPrimitiveShader();
    std::unique_ptr<ShaderImpl> createTextShader();
    std::unique_ptr<ShaderImpl> createBlurShader();
} // namespace p5cpp
