#include <p5cpp/p5cpp.hpp>

#include "application/engine.hpp"
#include "application/app_context.hpp"

#include "modules/input/input_data.hpp"
#include "modules/frame/frame_data.hpp"

#include "modules/frame/frame_module.hpp"
#include "modules/window/window_module.hpp"
#include "modules/input/input_module.hpp"
#include "modules/rendering/rendering_module.hpp"
#include "modules/sketch/sketch_module.hpp"

#include <cassert>

namespace p5cpp
{
    std::unique_ptr<Engine> engine;

    inline AppContext& getAppContext()
    {
        return engine->getContext();
    }
} // namespace p5cpp

namespace p5cpp
{
    void setWindowSize(int width, int height) { getAppContext().require<Window>().setSize(width, height); }
    void setWindowTitle(std::string_view title) { getAppContext().require<Window>().setTitle(title); }
    void setWindowResizable(bool resizable) { getAppContext().require<Window>().setResizable(resizable); }

    int getMouseX() { return getAppContext().require<InputData>().mouseX; }
    int getMouseY() { return getAppContext().require<InputData>().mouseY; }
    int getPMouseX() { return getAppContext().require<InputData>().pmouseX; }
    int getPMouseY() { return getAppContext().require<InputData>().pmouseY; }

    int getWidth() { return getAppContext().require<InputData>().logicalWidth; }
    int getHeight() { return getAppContext().require<InputData>().logicalHeight; }
    int getWindowWidth() { return getAppContext().require<InputData>().physicalWidth; }
    int getWindowHeight() { return getAppContext().require<InputData>().physicalHeight; }
} // namespace p5cpp

namespace p5cpp
{
    void frameRate(int targetFps) { getAppContext().require<FrameData>().targetFrameRate = targetFps; }
    void loop() { getAppContext().require<FrameData>().isPaused = false; }
    void noLoop() { getAppContext().require<FrameData>().isPaused = true; }
    bool isLooping() { return not getAppContext().require<FrameData>().isPaused; }
    void quit() { getAppContext().require<FrameData>().closeRequested = true; }
    void quit(int code) { exitCode(code), quit(); }
    void exitCode(int code) { getAppContext().require<FrameData>().exitCode = code; }
    int getFrameCount() { return getAppContext().require<FrameData>().frameCount; }
    int getFrameRate() { return getAppContext().require<FrameData>().framesPerSecond; }
    float getDeltaTime() { return getAppContext().require<FrameData>().deltaTime; }
    float getGlobalTime() { return getAppContext().require<FrameData>().globalTime; }
} // namespace p5cpp

namespace p5cpp
{
    UniformVariable uniform(float x) { return UniformVariable {.type = UniformVariable::Type::float1, .floatValue = x}; }
    UniformVariable uniform(float x, float y) { return UniformVariable {.type = UniformVariable::Type::float2, .float2Value = float2 {x, y}}; }
    UniformVariable uniform(float x, float y, float z, float w) { return UniformVariable {.type = UniformVariable::Type::float4, .float4Value = float4 {x, y, z, w}}; }
    UniformVariable uniform(const matrix4x4& value) { return UniformVariable {.type = UniformVariable::Type::matrix4x4, .matrix4x4Value = value}; }
} // namespace p5cpp

int main()
{
    using namespace p5cpp;

    engine = Engine::create();
    engine->addModule(std::make_unique<FrameModule>());
    engine->addModule(std::make_unique<WindowModule>());
    engine->addModule(std::make_unique<InputModule>());
    engine->addModule(std::make_unique<RenderingModule>());
    engine->addModule(std::make_unique<SketchModule>());

    engine->run();

    return 0;
}
