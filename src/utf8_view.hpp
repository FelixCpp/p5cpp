#pragma once

#include <string_view>

namespace p5
{
    struct Utf8View
    {
    };

    Utf8View utf8_view_create(std::string_view source);
} // namespace p5
