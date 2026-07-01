#include <p5cpp/application/logging.hpp>

#include <iostream>

namespace p5cpp
{
    void info(const std::string& message) { std::cout << "[INFO]: " << message << std::endl; }
    void debug(const std::string& message) { std::cout << "[DEBUG]: " << message << std::endl; }
    void warning(const std::string& message) { std::cout << "[WARNING]: " << message << std::endl; }
    void error(const std::string& message) { std::cerr << "[ERROR]: " << message << std::endl; }
} // namespace p5cpp
