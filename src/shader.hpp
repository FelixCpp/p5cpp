#pragma once

#include "p5.hpp"

namespace p5
{
    std::unique_ptr<Shader> create_default_shader();
    std::unique_ptr<Shader> create_text_shader();
} // namespace p5
