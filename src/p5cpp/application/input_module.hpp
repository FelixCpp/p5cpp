#pragma once

#include <p5cpp/application/module.hpp>
#include <p5cpp/application/input_component.hpp>

namespace p5cpp
{
    struct InputModule : public Module
    {
    public:
        void setup(AppContext& context, Next next) override;
        void event(AppContext& context, WindowEvent& event, Next next) override;
        void destroy(AppContext& context, Next next) override;

    private:
        InputComponent m_inputComponent;
    };
} // namespace p5cpp
