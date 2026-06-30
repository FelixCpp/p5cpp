#pragma once

#include "../module.hpp"
#include "../services/window.hpp"

namespace p5cpp
{
    class WindowModule : public Module
    {
    public:
        void setup(AppContext& context, Next next) override;
        void draw(AppContext& context, Next next) override;
        void destroy(AppContext& context, Next next) override;

    private:
        std::unique_ptr<Window> window;
    };
} // namespace p5cpp
