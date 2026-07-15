#include <p5cpp/application/engine.hpp>
#include <p5cpp/application/app_context.hpp>
#include <p5cpp/application/window.hpp>

#include <memory>

namespace p5cpp
{
    extern std::unique_ptr<Engine> engine;
}

namespace p5cpp
{
    Window& getWindow()
    {
        static Window* s_window = nullptr;
        static Engine* s_engine = nullptr;
        if (s_engine != engine.get()) {
            s_engine = engine.get();
            s_window = &engine->getContext().require<Window>();
        }
        return *s_window;
    }
} // namespace p5cpp

namespace p5cpp
{
    void setWindowSize(int width, int height) { getWindow().setSize(width, height); }
    void setWindowTitle(std::string_view title) { getWindow().setTitle(title); }
    void setWindowResizable(bool resizable) { getWindow().setResizable(resizable); }
} // namespace p5cpp
