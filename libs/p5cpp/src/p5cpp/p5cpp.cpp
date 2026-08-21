#include <p5cpp/p5cpp.hpp>
#include <p5cpp/application/lifecycle.hpp>
#include <p5cpp/application/lifecycle_plugin.hpp>
#include <p5cpp/application/sketch_plugin.hpp>
#include <p5cpp/graphics/graphics_plugin.hpp>
#include <p5cpp/window/window_plugin.hpp>
#include <p5cpp/input/input_plugin.hpp>
#include <p5cpp/application/kernel.hpp>

namespace p5
{
    inline static std::unique_ptr<Kernel> s_kernel;
    Kernel& getKernel()
    {
        return *s_kernel;
    }
} // namespace p5

namespace p5
{
    void send(const WindowEvent& event)
    {
        getKernel().process(event);
    }
} // namespace p5

int main()
{
    using namespace p5;

    Kernel::RunResult result;

    do {
        s_kernel = std::make_unique<Kernel>();

        s_kernel->addPlugin(std::make_unique<LifecyclePlugin>());
        s_kernel->addPlugin(std::make_unique<WindowPlugin>());
        s_kernel->addPlugin(std::make_unique<InputPlugin>());
        s_kernel->addPlugin(std::make_unique<GraphicsPlugin>());
        s_kernel->addPlugin(std::make_unique<SketchLoaderPlugin>());
        result = s_kernel->run();
    } while (result.shouldRestart);

    return result.exitCode;
}
