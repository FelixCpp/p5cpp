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
} // namespace p5
