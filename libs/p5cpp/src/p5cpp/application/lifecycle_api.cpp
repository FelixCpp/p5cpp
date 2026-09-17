#include <p5cpp/p5cpp.hpp>
#include <p5cpp/application/lifecycle.hpp>

namespace p5
{
    int getFrameCount()
    {
        return requireDependency<Lifecycle>().frameCount();
    }

    double getDeltaTime()
    {
        return requireDependency<Lifecycle>().deltaTime();
    }

    double getGlobalTime()
    {
        return requireDependency<Lifecycle>().globalTime();
    }

    void loop()
    {
        requireDependency<Lifecycle>().loop();
    }

    void noLoop()
    {
        requireDependency<Lifecycle>().noLoop();
    }

    bool isLooping()
    {
        return requireDependency<Lifecycle>().isLooping();
    }

    void redraw()
    {
        requireDependency<Lifecycle>().redraw();
    }

    void quit()
    {
        requireDependency<Lifecycle>().close();
    }

    void quit(int exitCode)
    {
        requireDependency<Lifecycle>().close(exitCode);
    }

    void setExitCode(int exitCode)
    {
        requireDependency<Lifecycle>().exitCode(exitCode);
    }

    void restart()
    {
        requireDependency<Lifecycle>().restart();
    }
} // namespace p5
