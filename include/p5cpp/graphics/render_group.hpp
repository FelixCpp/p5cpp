#pragma once

#include <memory>

namespace p5cpp
{
    struct RenderGroupImpl;

    // A handle to geometry pre-tessellated once by buildRenderGroup() and replayed cheaply
    // by drawRenderGroup(), instead of re-tessellating on every draw() call. Value type
    // wrapping a shared_ptr, mirroring Texture/Shader — cheap to copy, all copies alias the
    // same underlying (immutable) recorded geometry. `impl` is exposed directly: nothing
    // outside p5cpp's own sources has RenderGroupImpl's definition, so a raw shared_ptr to
    // it is inert to any external caller.
    struct RenderGroup
    {
        std::shared_ptr<const RenderGroupImpl> impl;
    };
} // namespace p5cpp
