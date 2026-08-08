#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/application/lifecycle.hpp>

namespace p5
{
    class LifecyclePlugin : public Plugin
    {
    public:
        LifecyclePlugin();

        void setup(Context& context, const Next& next) override;
        void event(Context& context, const Next& next, const WindowEvent& event) override;
        void draw(Context& context, const Next& next) override;
        void destroy(Context& context, const Next& next) override;

    private:
        Lifecycle m_lifecycle;
    };
} // namespace p5
