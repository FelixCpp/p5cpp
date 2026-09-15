#pragma once

#include <p5cpp/p5cpp.hpp>

extern "C"
{
#include <microui.h>
}

#include <memory>

namespace p5::gui
{
    mu_Context* getGuiContext();

    void beginGui();
    void endGui();

    template <typename Func>
        requires std::invocable<Func>
    void withGui(Func&& func);

    void setGuiTextSize(float pixels);
    float getGuiTextSize();

    void setGuiCornerRadius(float pixels);
    float getGuiCornerRadius();
} // namespace p5::gui

namespace p5::gui
{
    std::unique_ptr<Plugin> createGuiPlugin();
} // namespace p5::gui

namespace p5::gui
{
    template <typename Func>
        requires std::invocable<Func>
    inline void withGui(Func&& func)
    {
        try {
            beginGui();
            func();
            endGui();
        } catch (...) {
            endGui();
            throw;
        }
    }
} // namespace p5::gui
