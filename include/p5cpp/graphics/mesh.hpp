#pragma once

#include <p5cpp/graphics/color.hpp>

#include <p5cpp/math/value2.hpp>

namespace p5cpp
{
    // A single vertex for mesh() — raw, indexed triangle submission alongside p5cpp's
    // usual state-driven draw calls. Unlike fill()/stroke()-based shapes, position/uv/
    // color are always explicit per vertex: there is no "current geometry" to fall back
    // on. color defaults to opaque white, a no-op modulation for untextured/untinted use.
    struct MeshVertex
    {
        float2 position;
        float2 texcoord = {0.0f, 0.0f};
        color_t color = rgba(255);
    };
} // namespace p5cpp
