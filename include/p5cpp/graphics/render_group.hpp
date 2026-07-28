#pragma once

#include <memory>

namespace p5cpp
{
    struct RenderGroupImpl;
    class GraphicsComponent;

    // A handle to geometry pre-tessellated once by buildRenderGroup() and replayed cheaply
    // by drawRenderGroup(), instead of re-tessellating on every draw() call. Value type
    // wrapping a shared_ptr, mirroring Texture/Shader — cheap to copy, all copies alias the
    // same underlying (immutable) recorded geometry.
    class RenderGroup
    {
    public:
        RenderGroup();
        explicit RenderGroup(std::shared_ptr<const RenderGroupImpl> impl);

    private:
        friend class GraphicsComponent;

        const std::shared_ptr<const RenderGroupImpl>& getImpl() const;

        std::shared_ptr<const RenderGroupImpl> impl;
    };
} // namespace p5cpp
