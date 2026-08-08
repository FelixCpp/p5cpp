#include <p5cpp/p5cpp.hpp>

namespace p5
{
    void Plugin::setup(Context& context, const Next& next) { next(); }
    void Plugin::event(Context& context, const Next& next, const WindowEvent& event) { next(); }
    void Plugin::draw(Context& context, const Next& next) { next(); }
    void Plugin::destroy(Context& context, const Next& next) { next(); }
} // namespace p5
