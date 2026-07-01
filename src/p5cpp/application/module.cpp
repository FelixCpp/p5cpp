#include <p5cpp/application/module.hpp>

namespace p5cpp
{
    void Module::setup(AppContext& context, Next next)
    {
        next();
    }

    void Module::event(AppContext& context, WindowEvent& event, Next next)
    {
        next();
    }

    void Module::draw(AppContext& context, Next next)
    {
        next();
    }

    void Module::destroy(AppContext& context, Next next)
    {
        next();
    }
} // namespace p5cpp
