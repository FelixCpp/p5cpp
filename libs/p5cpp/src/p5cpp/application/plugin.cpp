#include <p5cpp/p5cpp.hpp>

namespace p5
{
    void Plugin::setup(const Next& next) { next(); }
    void Plugin::event(const Next& next, [[maybe_unused]] const WindowEvent& event) { next(); }
    void Plugin::draw(const Next& next) { next(); }
    void Plugin::destroy(const Next& next) { next(); }
} // namespace p5
