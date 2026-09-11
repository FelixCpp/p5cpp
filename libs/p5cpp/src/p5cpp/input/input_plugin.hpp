#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp/input/input.hpp>

namespace p5
{
    class InputPlugin : public Plugin
    {
    public:
        void setup(const Next& next);
        void event(const Next& next, const WindowEvent& event);
        void draw(const Next& next);
        void destroy(const Next& next);

    private:
        std::unique_ptr<Input> m_input;
    };
} // namespace p5
