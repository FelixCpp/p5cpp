#pragma once

#include <p5cpp/application/module.hpp>
#include <p5cpp/graphics/graphics_component.hpp>

namespace p5cpp
{
    class GraphicsModule : public Module
    {
    public:
        GraphicsModule();

        void setup(AppContext& context, Next next) override;
        void event(AppContext& context, WindowEvent& event, Next next) override;
        void draw(AppContext& context, Next next) override;
        void destroy(AppContext& context, Next next) override;

    private:
        GraphicsComponent m_component;
    };
} // namespace p5cpp
