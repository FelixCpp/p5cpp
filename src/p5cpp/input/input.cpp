#include <p5cpp/input/input.hpp>

namespace p5
{
    void Input::onEvent(const WindowEvent& event)
    {
        if (event.is<WindowEvent::KeyPress>()) {
            const auto& keyPress = event.as<WindowEvent::KeyPress>();
            m_keyDown[static_cast<size_t>(keyPress.key)] = true;
            if (not keyPress.repeat) {
                m_keyPressed[static_cast<size_t>(keyPress.key)] = true;
            }
        } else if (event.is<WindowEvent::KeyRelease>()) {
            const auto& keyRelease = event.as<WindowEvent::KeyRelease>();
            m_keyDown[static_cast<size_t>(keyRelease.key)] = false;
            m_keyReleased[static_cast<size_t>(keyRelease.key)] = true;
        } else if (event.is<WindowEvent::MouseButtonPress>()) {
            const auto& buttonPress = event.as<WindowEvent::MouseButtonPress>();
            m_buttonDown[static_cast<size_t>(buttonPress.button)] = true;
            m_buttonPressed[static_cast<size_t>(buttonPress.button)] = true;
            m_mouseX = buttonPress.x;
            m_mouseY = buttonPress.y;
        } else if (event.is<WindowEvent::MouseButtonRelease>()) {
            const auto& buttonRelease = event.as<WindowEvent::MouseButtonRelease>();
            m_buttonDown[static_cast<size_t>(buttonRelease.button)] = false;
            m_buttonReleased[static_cast<size_t>(buttonRelease.button)] = true;
            m_mouseX = buttonRelease.x;
            m_mouseY = buttonRelease.y;
        } else if (event.is<WindowEvent::MouseMove>()) {
            const auto& mouseMove = event.as<WindowEvent::MouseMove>();
            m_mouseX = mouseMove.x;
            m_mouseY = mouseMove.y;
        } else if (event.is<WindowEvent::MouseScroll>()) {
            const auto& mouseScroll = event.as<WindowEvent::MouseScroll>();
            m_scrollX += mouseScroll.xOffset;
            m_scrollY += mouseScroll.yOffset;
        } else if (event.is<WindowEvent::MouseEnter>()) {
            m_cursorInWindow = true;
        } else if (event.is<WindowEvent::MouseLeave>()) {
            m_cursorInWindow = false;
        } else if (event.is<WindowEvent::FileDrop>()) {
            m_droppedFiles = event.as<WindowEvent::FileDrop>().paths;
        }
    }

    void Input::endFrame()
    {
        m_keyPressed.fill(false);
        m_keyReleased.fill(false);
        m_buttonPressed.fill(false);
        m_buttonReleased.fill(false);
        m_scrollX = 0.0;
        m_scrollY = 0.0;
        m_droppedFiles.clear();
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
} // namespace p5
