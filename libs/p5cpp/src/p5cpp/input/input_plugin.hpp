#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/input/input.hpp>

namespace p5
{
    class InputPlugin : public Plugin
    {
    public:
        void setup(Context& context, const Next& next);
        void event(Context& context, const Next& next, const WindowEvent& event);
        void draw(Context& context, const Next& next);
        void destroy(Context& context, const Next& next);

    private:
        Input m_input;
    };
} // namespace p5
