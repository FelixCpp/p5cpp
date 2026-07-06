#include <p5cpp/p5cpp.hpp>

#include <p5cpp/application/engine.hpp>
#include <p5cpp/application/app_context.hpp>
#include <p5cpp/application/frame_module.hpp>
#include <p5cpp/application/window_module.hpp>
#include <p5cpp/application/input_module.hpp>
#include <p5cpp/application/sketch_module.hpp>

#include <p5cpp/graphics/graphics_module.hpp>

#include <cassert>

namespace p5cpp
{
    std::unique_ptr<Engine> engine;
} // namespace p5cpp

int main()
{
    using namespace p5cpp;

    engine = Engine::create();
    engine->addModule(std::make_unique<FrameModule>());
    engine->addModule(std::make_unique<WindowModule>());
    engine->addModule(std::make_unique<InputModule>());
    engine->addModule(std::make_unique<GraphicsModule>());
    engine->addModule(std::make_unique<SketchModule>());

    engine->run();

    return 0;
}
