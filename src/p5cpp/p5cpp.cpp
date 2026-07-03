#include <p5cpp/p5cpp.hpp>

#include <p5cpp/application/engine.hpp>
#include <p5cpp/application/frame_module.hpp>
#include <p5cpp/application/window_module.hpp>
#include <p5cpp/application/sketch_module.hpp>

namespace p5cpp
{
    std::unique_ptr<Engine> engine;
}

int main()
{
    using namespace p5cpp;

    engine = Engine::create();
    engine->addModule(std::make_unique<FrameModule>());
    engine->addModule(std::make_unique<WindowModule>());
    engine->addModule(std::make_unique<SketchModule>());

    engine->run();
}
