#pragma once

#include <p5cpp/p5cpp.hpp>

namespace p5
{
    struct GraphicsImpl
    {
        uint32_t id = 0;                         // resolve FBO — holds colorTexture, always readable/sample-able
        uint32_t depthStencilRenderbufferId = 0; // currently unused by the rendering pipeline (clip() uses
                                                 // glScissor, not the stencil buffer); kept for FBO completeness
        uint32_t msaaFramebufferId = 0;          // 0 = no antialiasing; else the FBO actually rendered into
        uint32_t msaaColorRenderbufferId = 0;

        GraphicsImpl() = default;
        GraphicsImpl(const GraphicsImpl&) = delete;
        GraphicsImpl& operator=(const GraphicsImpl&) = delete;
        ~GraphicsImpl();
    };
} // namespace p5
