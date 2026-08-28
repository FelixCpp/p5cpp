#include <p5cpp/p5cpp.hpp>
#include <p5cpp/application/kernel.hpp>
#include <p5cpp/application/lifecycle.hpp>

namespace p5
{
    int getFrameCount()
    {
        return getKernel().getContext().require<Lifecycle>().frameCount();
    }

    double getDeltaTime()
    {
        return getKernel().getContext().require<Lifecycle>().deltaTime();
    }

    double getGlobalTime()
    {
        return getKernel().getContext().require<Lifecycle>().globalTime();
    }

    void loop()
    {
        getKernel().getContext().require<Lifecycle>().loop();
    }

    void noLoop()
    {
        getKernel().getContext().require<Lifecycle>().noLoop();
    }

    bool isLooping()
    {
        return getKernel().getContext().require<Lifecycle>().isLooping();
    }

    void redraw()
    {
        getKernel().getContext().require<Lifecycle>().redraw();
    }

    void quit()
    {
        getKernel().getContext().require<Lifecycle>().close();
    }

    void quit(int exitCode)
    {
        getKernel().getContext().require<Lifecycle>().close(exitCode);
    }

    void setExitCode(int exitCode)
    {
        getKernel().getContext().require<Lifecycle>().exitCode(exitCode);
    }
} // namespace p5
