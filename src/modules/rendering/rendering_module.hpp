#pragma once

#include "../../module.hpp"
#include "rendering_data.hpp"

namespace p5cpp
{
    class RenderingModule : public Module
    {
    public:
        void setup(AppContext& context, Next next) override;
        void event(AppContext& context, WindowEvent& event, Next next) override;
        void draw(AppContext& context, Next next) override;
        void destroy(AppContext& context, Next next) override;

    private:
        RenderingData data;

        bool needsDefaultCanvasRecreation;
    };
} // namespace p5cpp
