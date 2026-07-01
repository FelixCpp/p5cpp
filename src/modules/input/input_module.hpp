#pragma once

#include <p5cpp/application/module.hpp>

#include "input_data.hpp"

namespace p5cpp
{
    struct InputModule : public Module
    {
    public:
        void setup(AppContext& context, Next next) override;
        void event(AppContext& context, WindowEvent& event, Next next) override;

    private:
        InputData data;
    };
} // namespace p5cpp
