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
        static Engine* s_engine = nullptr;
        if (s_engine != engine.get()) {
            s_engine = engine.get();
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

    bool isKeyDown(Key key) { return getInputComponent().isKeyDown(key); }
    bool isKeyPressed(Key key) { return getInputComponent().isKeyPressed(key); }
    bool isKeyReleased(Key key) { return getInputComponent().isKeyReleased(key); }

    bool isMouseDown(MouseButton button) { return getInputComponent().isMouseDown(button); }
    bool isMousePressed(MouseButton button) { return getInputComponent().isMousePressed(button); }
    bool isMouseReleased(MouseButton button) { return getInputComponent().isMouseReleased(button); }

    bool isMouseDragging() { return getInputComponent().isMouseDragging(); }
    bool isMouseDragging(MouseButton button) { return getInputComponent().isMouseDragging(button); }

    float getScrollX() { return getInputComponent().getScrollX(); }
    float getScrollY() { return getInputComponent().getScrollY(); }

    std::span<const char32_t> getCharsTyped() { return getInputComponent().getCharsTyped(); }
    std::span<const std::filesystem::path> getDroppedFiles() { return getInputComponent().getDroppedFiles(); }
} // namespace p5cpp
