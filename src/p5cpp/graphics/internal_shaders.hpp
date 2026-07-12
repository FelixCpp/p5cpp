#pragma once

#include <p5cpp/graphics/shader.hpp>

namespace p5cpp
{
    std::unique_ptr<ShaderImpl> createPrimitiveShader();
    std::unique_ptr<ShaderImpl> createTextShader();
    std::unique_ptr<ShaderImpl> createBlurShader();
    std::unique_ptr<ShaderImpl> createGrayscaleShader();
    std::unique_ptr<ShaderImpl> createInvertShader();
    std::unique_ptr<ShaderImpl> createThresholdShader();
} // namespace p5cpp
