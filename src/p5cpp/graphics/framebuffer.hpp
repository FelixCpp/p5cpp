#pragma once

#include <p5cpp/p5cpp.hpp>

namespace p5
{
    void blitFramebufferToScreen(const std::shared_ptr<Framebuffer>& framebuffer, uint32_t screenWidth, uint32_t screenHeight);
} // namespace p5
