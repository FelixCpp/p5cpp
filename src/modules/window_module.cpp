#include "window_module.hpp"
#include "../app_context.hpp"
#include "../engine.hpp"

namespace p5cpp
{
    void WindowModule::setup(AppContext& context, std::function<void()> next)
    {
        window = Window::create(800, 600, "p5cpp");
        window->setEventCallback([this, engine = context.engine](const WindowEvent& event) {
            engine->dispatch(event);
        });

        next();

        window->setVisible(true);
    }

    void WindowModule::draw(AppContext& context, std::function<void()> next)
    {
        window->pollEvents();
        next();
        window->swapBuffers();
    }

    void WindowModule::destroy(AppContext& context, std::function<void()> next)
    {
        next();
        window.reset();
    }
} // namespace p5cpp
