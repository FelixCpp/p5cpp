#include "p5.hpp"

namespace p5
{
    // clang-format off
    const matrix4x4 matrix4x4::identity = {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
    };
    // clang-format on
} // namespace p5
