#pragma once

#include <functional>

namespace p5cpp
{
    typedef std::function<void()> Next;
}

namespace p5cpp
{
    struct AppContext;
    struct WindowEvent;

    struct Module
    {
        virtual ~Module() = default;

        virtual void setup(AppContext& context, Next next);
        virtual void event(AppContext& context, WindowEvent& event, Next next);
        virtual void draw(AppContext& context, Next next);
        virtual void destroy(AppContext& context, Next next);
    };
} // namespace p5cpp
