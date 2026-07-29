#pragma once

#include <p5cpp/graphics/shader.hpp>

namespace p5cpp
{
    Shader createPrimitiveShader();
    Shader createTextShader();
    Shader createBlurShader();
    Shader createGrayscaleShader();
    Shader createInvertShader();
    Shader createThresholdShader();
} // namespace p5cpp
