#pragma once

#include "../module.hpp"

namespace p5cpp
{
    class LifecycleModule : public Module
    {
    public:
        void event(AppContext& context, WindowEvent& event, Next next) override;
        void draw(AppContext& context, Next next) override;
    };
} // namespace p5cpp
