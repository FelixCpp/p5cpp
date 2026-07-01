#pragma once

#include <string>

namespace p5cpp
{
    void info(const std::string& message);
    void debug(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
} // namespace p5cpp
