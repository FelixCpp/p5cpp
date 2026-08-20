#include <p5cpp/p5cpp.hpp>
#include <p5cpp/application/kernel.hpp>
#include <p5cpp/input/input.hpp>

namespace p5
{
    namespace
    {
        Input& input()
        {
            return getKernel().getContext().require<Input>();
        }
    } // namespace

    bool isKeyDown(Key key)
    {
        return input().isKeyDown(key);
    }

    bool isKeyPressed(Key key)
    {
        return input().isKeyPressed(key);
    }

    bool isKeyReleased(Key key)
    {
        return input().isKeyReleased(key);
    }

    bool isMouseButtonDown(MouseButton button)
    {
        return input().isMouseButtonDown(button);
    }

    bool isMouseButtonPressed(MouseButton button)
    {
        return input().isMouseButtonPressed(button);
    }

    bool isMouseButtonReleased(MouseButton button)
    {
        return input().isMouseButtonReleased(button);
    }

    double getMouseX()
    {
        return input().mouseX();
    }

    double getMouseY()
    {
        return input().mouseY();
    }

    double getPreviousMouseX()
    {
        return input().previousMouseX();
    }

    double getPreviousMouseY()
    {
        return input().previousMouseY();
    }

    double getMouseDeltaX()
    {
        return input().mouseX() - input().previousMouseX();
    }

    double getMouseDeltaY()
    {
        return input().mouseY() - input().previousMouseY();
    }

    double getScrollX()
    {
        return input().scrollX();
    }

    double getScrollY()
    {
        return input().scrollY();
    }

    bool isCursorInWindow()
    {
        return input().cursorInWindow();
    }

    std::span<const std::string> getDroppedFiles()
    {
        return input().droppedFiles();
    }

    std::span<const uint32_t> getTypedChars()
    {
        return input().typedChars();
    }
} // namespace p5
