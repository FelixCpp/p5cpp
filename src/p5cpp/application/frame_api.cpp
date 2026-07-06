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
    inline static FrameComponent& getFrameComponent()
    {
        return engine->getContext().require<FrameComponent>();
    }
} // namespace p5cpp

namespace p5cpp
{
    void frameRate(int frameRate) { getFrameComponent().frameRate(frameRate); }
    void loop() { getFrameComponent().loop(); }
    void noLoop() { getFrameComponent().noLoop(); }
    bool isLooping() { return getFrameComponent().isLooping(); }
    void quit() { getFrameComponent().quit(); }
    void quit(int code) { getFrameComponent().quit(code); }
    void exitCode(int code) { getFrameComponent().exitCode(code); }
    int getFrameCount() { return getFrameComponent().getFrameCount(); }
    int getFrameRate() { return getFrameComponent().getFrameRate(); }
    float getDeltaTime() { return getFrameComponent().getDeltaTime(); }
    float getGlobalTime() { return getFrameComponent().getGlobalTime(); }
} // namespace p5cpp
