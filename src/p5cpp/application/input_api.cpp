#include <p5cpp/application/input_component.hpp>
#include <p5cpp/application/engine.hpp>
#include <p5cpp/application/app_context.hpp>

namespace p5cpp
{
    extern std::unique_ptr<Engine> engine;
}

namespace p5cpp
{
    InputComponent& getInputComponent()
    {
        static InputComponent* s_inputComponent = nullptr;
        if (s_inputComponent == nullptr) {
            s_inputComponent = &engine->getContext().require<InputComponent>();
        }
        return *s_inputComponent;
    }
} // namespace p5cpp

namespace p5cpp
{
    int getMouseX() { return getInputComponent().getMouseX(); }
    int getMouseY() { return getInputComponent().getMouseY(); }
    int getPMouseX() { return getInputComponent().getPMouseX(); }
    int getPMouseY() { return getInputComponent().getPMouseY(); }
    int getLogicalWidth() { return getInputComponent().getLogicalWidth(); }
    int getLogicalHeight() { return getInputComponent().getLogicalHeight(); }
    int getPhysicalWidth() { return getInputComponent().getPhysicalWidth(); }
    int getPhysicalHeight() { return getInputComponent().getPhysicalHeight(); }
} // namespace p5cpp
