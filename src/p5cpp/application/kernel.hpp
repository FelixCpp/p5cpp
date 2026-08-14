#pragma once

#include <p5cpp/p5cpp.hpp>

#include <vector>

namespace p5
{
    class Kernel
    {
    public:
        struct RunResult
        {
            bool shouldRestart;
            int exitCode;
        };

        void addPlugin(std::unique_ptr<Plugin> plugin);
        void addPluginsAndSetup(std::vector<std::unique_ptr<Plugin>> plugins);
        void process(const WindowEvent& event);
        RunResult run();

        Context& getContext();

    private:
        void dispatchSetup();
        void dispatchEvent(const WindowEvent& event);
        void dispatchDraw();
        void dispatchDestroy();

    private:
        std::vector<std::unique_ptr<Plugin>> m_plugins;
        Context m_context;
    };

    Kernel& getKernel();
} // namespace p5
