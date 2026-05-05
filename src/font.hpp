#pragma once

#include <filesystem>
#include <memory>

#include "p5.hpp"

namespace p5
{
    std::unique_ptr<Font> loadFont(const std::filesystem::path& fontFilePath, int size);
} // namespace p5
