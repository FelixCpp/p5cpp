#include <p5cpp/input/input.hpp>

namespace
{
    template <typename... Ts> struct Overloaded : Ts...
    {
        using Ts::operator()...;
    };
} // namespace

namespace p5
{
    void Input::process(const WindowEvent& event)
    {
        event.visit(Overloaded {
            [this](const WindowEvent::KeyPress& keyPress) {
                m_keyDown[static_cast<size_t>(keyPress.key)] = true;
                if (not keyPress.repeat) {
                    m_keyPressed[static_cast<size_t>(keyPress.key)] = true;
                }
            },
            [this](const WindowEvent::KeyRelease& keyRelease) {
                m_keyDown[static_cast<size_t>(keyRelease.key)] = false;
                m_keyReleased[static_cast<size_t>(keyRelease.key)] = true;
            },
            [this](const WindowEvent::MouseButtonPress& buttonPress) {
                const auto buttonIndex = static_cast<size_t>(buttonPress.button);
                m_buttonDown[buttonIndex] = true;
                m_buttonPressed[buttonIndex] = true;
                m_mouseX = buttonPress.x;
                m_mouseY = buttonPress.y;
            },
            [this](const WindowEvent::MouseButtonRelease& buttonRelease) {
                const auto buttonIndex = static_cast<size_t>(buttonRelease.button);
                m_buttonDown[buttonIndex] = false;
                m_buttonReleased[buttonIndex] = true;
                m_mouseX = buttonRelease.x;
                m_mouseY = buttonRelease.y;
            },
            [this](const WindowEvent::MouseMove& mouseMove) {
                m_mouseX = mouseMove.x;
                m_mouseY = mouseMove.y;
            },
            [this](const WindowEvent::MouseScroll& mouseScroll) {
                m_scrollX += mouseScroll.xOffset;
                m_scrollY += mouseScroll.yOffset;
            },
            [this](const WindowEvent::MouseEnter&) {
                m_cursorInWindow = true;
            },
            [this](const WindowEvent::MouseLeave&) {
                m_cursorInWindow = false;
            },
            [this](const WindowEvent::FileDrop& fileDrop) {
                m_droppedFiles = fileDrop.paths;
            },
            [this](const WindowEvent::CharInput& charInput) {
                m_typedChars.push_back(charInput.codepoint);
            },
            [](const auto&) {
            },
        });
    }

    void Input::reset()
    {
        m_keyPressed.fill(false);
        m_keyReleased.fill(false);
        m_buttonPressed.fill(false);
        m_buttonReleased.fill(false);
        m_scrollX = 0.0;
        m_scrollY = 0.0;
        m_droppedFiles.clear();
        m_typedChars.clear();
        m_prevMouseX = m_mouseX;
        m_prevMouseY = m_mouseY;
    }

    bool Input::isKeyDown(Key key) const
    {
        return m_keyDown[static_cast<size_t>(key)];
    }

    bool Input::isKeyPressed(Key key) const
    {
        return m_keyPressed[static_cast<size_t>(key)];
    }

    bool Input::isKeyReleased(Key key) const
    {
        return m_keyReleased[static_cast<size_t>(key)];
    }

    bool Input::isMouseButtonDown(MouseButton button) const
    {
        return m_buttonDown[static_cast<size_t>(button)];
    }

    bool Input::isMouseButtonPressed(MouseButton button) const
    {
        return m_buttonPressed[static_cast<size_t>(button)];
    }

    bool Input::isMouseButtonReleased(MouseButton button) const
    {
        return m_buttonReleased[static_cast<size_t>(button)];
    }

    double Input::mouseX() const
    {
        return m_mouseX;
    }

    double Input::mouseY() const
    {
        return m_mouseY;
    }

    double Input::previousMouseX() const
    {
        return m_prevMouseX;
    }

    double Input::previousMouseY() const
    {
        return m_prevMouseY;
    }

    double Input::scrollX() const
    {
        return m_scrollX;
    }

    double Input::scrollY() const
    {
        return m_scrollY;
    }

    bool Input::cursorInWindow() const
    {
        return m_cursorInWindow;
    }

    std::span<const std::string> Input::droppedFiles() const
    {
        return m_droppedFiles;
    }

    std::span<const uint32_t> Input::typedChars() const
    {
        return m_typedChars;
    }
} // namespace p5
