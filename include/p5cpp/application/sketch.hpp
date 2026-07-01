#pragma once

#include <p5cpp/application/window_event.hpp>

#include <memory>

namespace p5cpp
{
    struct Sketch
    {
        virtual ~Sketch() = default;
        virtual void setup() = 0;
        virtual void event(const WindowEvent& event) {}
        virtual void draw() = 0;
        virtual void destroy() {}
    };

    extern std::unique_ptr<Sketch> createSketch();
} // namespace p5cpp
