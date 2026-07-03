#include <p5cpp/application/frame_component.hpp>
#include <p5cpp/application/engine.hpp>
#include <p5cpp/application/app_context.hpp>

#include <memory>

namespace p5cpp
{
    extern std::unique_ptr<Engine> engine;
}

namespace p5cpp
{
    void frameRate(int frameRate)
    {
        FrameComponent& component = engine->getContext().require<FrameComponent>();
        component.frameRate(frameRate);
    }

    void loop()
    {
        FrameComponent& component = engine->getContext().require<FrameComponent>();
        component.loop();
    }

    void noLoop()
    {
        FrameComponent& component = engine->getContext().require<FrameComponent>();
        component.noLoop();
    }

    bool isLooping()
    {
        FrameComponent& component = engine->getContext().require<FrameComponent>();
        return component.isLooping();
    }

    void quit()
    {
        FrameComponent& component = engine->getContext().require<FrameComponent>();
        component.close();
    }

    void quit(int code)
    {
        FrameComponent& component = engine->getContext().require<FrameComponent>();
        component.close(code);
    }

    void exitCode(int code)
    {
        FrameComponent& component = engine->getContext().require<FrameComponent>();
        component.exitCode(code);
    }

    int getFrameCount()
    {
        FrameComponent& component = engine->getContext().require<FrameComponent>();
        return component.getFrameCount();
    }

    int getFrameRate()
    {
        FrameComponent& component = engine->getContext().require<FrameComponent>();
        return component.getFrameRate();
    }

    float getDeltaTime()
    {
        FrameComponent& component = engine->getContext().require<FrameComponent>();
        return component.getDeltaTime();
    }

    float getGlobalTime()
    {
        FrameComponent& component = engine->getContext().require<FrameComponent>();
        return component.getGlobalTime();
    }
} // namespace p5cpp
