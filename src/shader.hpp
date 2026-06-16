#pragma once

#include <p5cpp.hpp>

namespace p5cpp
{
    std::unique_ptr<Shader> create_default_shader();
    std::unique_ptr<Shader> create_text_shader();
} // namespace p5cpp
