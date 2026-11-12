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

    void setFrameRate(double framesPerSecond)
    {
        requireDependency<Lifecycle>().setFrameRate(framesPerSecond);
    }

    void noFrameRateLimit()
    {
        requireDependency<Lifecycle>().setFrameRate(0.0);
    }

    double getFrameRate()
    {
        return requireDependency<Lifecycle>().frameRate();
    }

    double getMeasuredFrameRate()
    {
        return requireDependency<Lifecycle>().measuredFrameRate();
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

    void setQuitOnEscape(bool enabled)
    {
        requireDependency<Lifecycle>().quitOnEscape(enabled);
    }
} // namespace p5
