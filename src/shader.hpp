#pragma once

#include "p5.hpp"

namespace p5
{
    std::unique_ptr<Shader> createDefaultShader();
    std::unique_ptr<Shader> createTextShader();
} // namespace p5
