#include <p5cpp/p5cpp.hpp>

namespace p5
{
    void Plugin::setup([[maybe_unused]] Context& context, const Next& next) { next(); }
    void Plugin::event([[maybe_unused]] Context& context, const Next& next, [[maybe_unused]] const WindowEvent& event) { next(); }
    void Plugin::draw([[maybe_unused]] Context& context, const Next& next) { next(); }
    void Plugin::destroy([[maybe_unused]] Context& context, const Next& next) { next(); }
} // namespace p5
