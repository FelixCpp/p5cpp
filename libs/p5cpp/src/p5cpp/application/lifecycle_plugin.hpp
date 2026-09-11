#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/application/lifecycle.hpp>

namespace p5
{
    class LifecyclePlugin : public Plugin
    {
    public:
        LifecyclePlugin();

        void setup(const Next& next) override;
        void event(const Next& next, const WindowEvent& event) override;
        void draw(const Next& next) override;
        void destroy(const Next& next) override;

    private:
        std::unique_ptr<Lifecycle> m_lifecycle;
    };
} // namespace p5
