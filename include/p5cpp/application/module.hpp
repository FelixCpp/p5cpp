#pragma once

#include <functional>
#include <memory>

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

    struct ModuleRegistrar
    {
        virtual ~ModuleRegistrar() = default;

        virtual void addModuleBefore(std::unique_ptr<Module> module) = 0;
        virtual void addModuleAfter(std::unique_ptr<Module> module) = 0;
    };
} // namespace p5cpp
