#pragma once

#include <p5cpp.hpp>

#include <functional>
#include <memory>
#include <string_view>

namespace p5cpp
{
    struct Window
    {
        static std::unique_ptr<Window> create(int width, int height, std::string_view title);

        virtual ~Window() = default;

        virtual void setSize(int width, int height) = 0;
        virtual void setPosition(int x, int y) = 0;
        virtual void setTitle(std::string_view title) = 0;
        virtual void setResizable(bool resizable) = 0;

        virtual void setEventCallback(std::function<void(const WindowEvent&)> callback) = 0;
        virtual void setVisible(bool visible) = 0;
        virtual void swapBuffers() = 0;
        virtual void pollEvents() = 0;

        virtual int2 getMousePosition() = 0;
        virtual int2 getLogicalSize() = 0;
        virtual int2 getPhysicalSize() = 0;
    };
} // namespace p5cpp
