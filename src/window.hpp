#pragma once

#include "p5.hpp"
#include <functional>
#include <string_view>

namespace p5
{
    // Opaque handle to the application window. Not exposed to sketches.
    struct AppWindow;

    using WindowEventCallback = std::function<void(const WindowEvent&)>;

    // Creates the GLFW window, loads OpenGL via GLAD, and registers all GLFW callbacks.
    // onEvent is called for every input and resize event from this point on.
    AppWindow* window_create(int width, int height, const char* title, WindowEventCallback onEvent);
    void window_destroy(AppWindow* win);

    void window_show(AppWindow* win);
    bool window_should_close(const AppWindow* win);
    void window_poll_events(AppWindow* win);
    void window_swap_buffers(AppWindow* win);

    // Backing implementations for the public p5 window API.
    void window_set_size(AppWindow* win, int width, int height);
    void window_set_title(AppWindow* win, std::string_view title);
    void window_set_resizable(AppWindow* win, bool resizable);
    int window_logical_width(const AppWindow* win);
    int window_logical_height(const AppWindow* win);
    int window_physical_width(const AppWindow* win);
    int window_physical_height(const AppWindow* win);

} // namespace p5
