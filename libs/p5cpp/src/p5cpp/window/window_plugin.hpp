#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/window/window.hpp>

namespace p5
{
    class WindowPlugin : public Plugin
    {
    public:
        WindowPlugin();

        void setup(Context& context, const Next& next);
        void event(Context& context, const Next& next, const WindowEvent& event);
        void draw(Context& context, const Next& next);
        void destroy(Context& context, const Next& next);

    private:
        std::unique_ptr<Window> m_window;
    };
} // namespace p5
