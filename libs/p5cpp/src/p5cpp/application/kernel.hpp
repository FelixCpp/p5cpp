#pragma once

#include <p5cpp/p5cpp.hpp>

#include <deque>
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
        // deque, not vector: Next holds a view over this container across an in-progress dispatch
        // chain (see Next::m_chain), and a plugin's setup() can itself call addPluginsAndSetup() --
        // e.g. SketchLoaderPlugin::setup() always does, appending at least the SketchPlugin -- which
        // appends to this same container from *inside* that outer dispatch. push_back() on a vector
        // can reallocate its backing storage, which would dangle the outer Next's view into freed
        // memory; deque::push_back() never invalidates references to existing elements, so the outer
        // chain's already-captured view stays valid regardless of what setup() adds during dispatch.
        std::deque<std::unique_ptr<Plugin>> m_plugins;
        Context m_context;
    };

    Kernel& getKernel();
} // namespace p5
