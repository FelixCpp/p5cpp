#pragma once

#include "../module.hpp"
#include "../services/window.hpp"

namespace p5cpp
{
    class WindowModule : public Module
    {
    public:
        void setup(AppContext& context, std::function<void()> next) override;
        void draw(AppContext& context, std::function<void()> next) override;
        void destroy(AppContext& context, std::function<void()> next) override;

    private:
        std::unique_ptr<Window> window;
    };
} // namespace p5cpp
