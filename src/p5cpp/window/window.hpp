#pragma once

#include <p5cpp/p5cpp.hpp>

#include <GLFW/glfw3.h>

#include <memory>
#include <string_view>
#include <functional>

namespace p5
{
    class Window
    {
    public:
        using EventCallback = std::function<void(const WindowEvent&)>;

        static std::unique_ptr<Window> create(uint32_t width, uint32_t height, std::string_view title, const EventCallback& eventCallback);
        ~Window();

        void setSize(uint32_t width, uint32_t height);
        void setPosition(int32_t x, int32_t y);
        void setTitle(std::string_view title);
        void setResizable(bool resizable);
        void setVisible(bool visible);

        void pollEvents();
        void swapBuffers();

    private:
        explicit Window(GLFWwindow* window, const EventCallback& eventCallback);

        void publish(const WindowEvent& event);

        GLFWwindow* m_window;
        EventCallback m_eventCallback;
    };
} // namespace p5
