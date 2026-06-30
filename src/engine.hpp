#pragma once

#include "module.hpp"

#include <memory>

namespace p5cpp
{
    struct Engine
    {
        static std::unique_ptr<Engine> create();

        virtual ~Engine() = default;

        virtual void addModule(std::unique_ptr<Module> module) = 0;
        virtual void dispatch(const WindowEvent& event) = 0;
        virtual void run() = 0;
    };
} // namespace p5cpp
