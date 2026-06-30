#pragma once

#include "../module.hpp"

namespace p5cpp
{
    struct InputModule : public Module
    {
        void setup(AppContext& context, Next next) override;
        void event(AppContext& context, WindowEvent& event, Next next) override;
    };
} // namespace p5cpp
