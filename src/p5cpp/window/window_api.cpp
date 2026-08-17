#include <p5cpp/p5cpp.hpp>
#include <p5cpp/application/kernel.hpp>
#include <p5cpp/window/window.hpp>

namespace p5
{
    namespace
    {
        Window& window()
        {
            return getKernel().getContext().require<Window>();
        }
    } // namespace

    void setWindowSize(uint32_t width, uint32_t height) { window().setSize(width, height); }
    void setWindowPosition(int32_t x, int32_t y) { window().setPosition(x, y); }
    void setWindowTitle(std::string_view title) { window().setTitle(title); }
    void setWindowResizable(bool resizable) { window().setResizable(resizable); }
    void setWindowVisible(bool visible) { window().setVisible(visible); }
    void maximizeWindow() { window().maximize(); }
    bool isWindowMaximized() { return window().isMaximized(); }
    void minimizeWindow() { window().minimize(); }
    bool isWindowMinimized() { return window().isMinimized(); }
    void enterFullscreen() { window().enterFullscreen(); }
    void leaveFullscreen(std::optional<rect2i> restoreRect) { window().leaveFullscreen(restoreRect); }
    void toggleFullscreen() { window().toggleFullscreen(); }
    bool isWindowFullscreen() { return window().isFullscreen(); }
    uint2 getWindowSize() { return window().getLogicalSize(); }
    uint2 getWindowPhysicalSize() { return window().getPhysicalSize(); }
    int2 getWindowPosition() { return window().getPosition(); }
    std::string_view getWindowTitle() { return window().getTitle(); }
    bool isWindowResizable() { return window().isResizable(); }
    bool isWindowVisible() { return window().isVisible(); }
} // namespace p5
