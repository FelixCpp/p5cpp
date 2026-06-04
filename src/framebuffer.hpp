#pragma once

#include "p5.hpp"

namespace p5
{
    // Creates an off-screen canvas with separate physical (texture) and logical (user-coord) sizes.
    // Used internally for the default window canvas, where HiDPI causes them to differ.
    std::unique_ptr<Framebuffer> create_window_framebuffer(int physWidth, int physHeight, int logicalWidth, int logicalHeight);

    // Blit the off-screen default canvas to FBO 0 (the OS window surface) without Y-flip.
    void blit_framebuffer_to_screen(const Framebuffer& source);
} // namespace p5
