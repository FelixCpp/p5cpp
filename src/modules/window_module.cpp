#include "window_module.hpp"
#include "../app_context.hpp"
#include "../engine.hpp"

namespace p5cpp
{
    void WindowModule::setup(AppContext& context, Next next)
    {
        info("WindowModule setup");

        window = Window::create(800, 600, "p5cpp");
        window->setEventCallback([this, engine = context.engine](const WindowEvent& event) {
            engine->dispatch(event);
        });

        context.window = window.get();

        next();

        window->setVisible(true);
    }

    void WindowModule::draw(AppContext& context, Next next)
    {
        window->pollEvents();
        next();
        window->swapBuffers();
    }

    void WindowModule::destroy(AppContext& context, Next next)
    {
        next();

        context.window = nullptr;
        window.reset();
    }
} // namespace p5cpp
