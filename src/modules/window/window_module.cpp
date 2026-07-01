#include "window_module.hpp"

#include "../../application/engine.hpp"
#include "../../application/app_context.hpp"

#include <p5cpp/application/logging.hpp>

namespace p5cpp
{
    void WindowModule::setup(AppContext& context, Next next)
    {
        info("WindowModule setup");

        Engine& engine = context.require<Engine>();
        window = Window::create(800, 600, "p5cpp");
        window->setEventCallback([this, &engine](const WindowEvent& event) {
            engine.dispatch(event);
        });

        context.registerService<Window>(window.get());

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

        context.unregisterService<Window>();

        window.reset();
    }
} // namespace p5cpp
