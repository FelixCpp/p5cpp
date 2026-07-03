#pragma once

#include <p5cpp/application/module.hpp>
#include <p5cpp/application/frame_component.hpp>

namespace p5cpp
{
    class FrameModule : public Module
    {
    public:
        void setup(AppContext& context, Next next) override;
        void event(AppContext& context, WindowEvent& event, Next next) override;
        void draw(AppContext& context, Next next) override;
        void destroy(AppContext& context, Next next) override;

    private:
        FrameComponent m_frameComponent;
    };
} // namespace p5cpp
