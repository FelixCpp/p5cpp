#pragma once

#include "../module.hpp"
#include "p5cpp.hpp"

namespace p5cpp
{
    class SketchModule : public Module
    {
    public:
        void setup(AppContext& context, Next next) override;
        void event(AppContext& context, WindowEvent& event, Next next) override;
        void draw(AppContext& context, Next next) override;
        void destroy(AppContext& context, Next next) override;

    private:
        std::unique_ptr<Sketch> sketch;
    };
} // namespace p5cpp
