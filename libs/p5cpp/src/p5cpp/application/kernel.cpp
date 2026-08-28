#include <p5cpp/application/kernel.hpp>
#include <p5cpp/application/lifecycle.hpp>

namespace p5
{
    void Kernel::addPlugin(std::unique_ptr<Plugin> plugin)
    {
        m_plugins.push_back(std::move(plugin));
    }

    void Kernel::addPluginsAndSetup(std::vector<std::unique_ptr<Plugin>> plugins)
    {
        const size_t startIndex = m_plugins.size();

        for (auto& plugin : plugins) {
            m_plugins.push_back(std::move(plugin));
        }

        const auto next = Next {
            m_plugins,
            startIndex,
            &m_context,
            [](Plugin& plugin, Context& context, const Next& next) {
                plugin.setup(context, next);
            },
            nullptr,
        };

        next();
    }

    void Kernel::process(const WindowEvent& event)
    {
        dispatchEvent(event);
    }

    Context& Kernel::getContext()
    {
        return m_context;
    }

    Kernel::RunResult Kernel::run()
    {
        try {
            dispatchSetup();

            Lifecycle& lifecycle = m_context.require<Lifecycle>();

            while (not lifecycle.shouldClose()) {
                // dispatchDraw() always runs (WindowPlugin::draw() still needs to pump events and swap
                // buffers so the window stays responsive). nextFrame() itself decides whether this frame
                // is actually drawn -- isLooping(), or a pending redraw() -- and only then advances
                // frameCount/deltaTime/globalTime; that decision is read back via shouldDrawThisFrame(),
                // which gates the sketch's draw() call in SketchPlugin::draw().
                lifecycle.nextFrame();
                dispatchDraw();
            }

            RunResult result {
                .shouldRestart = lifecycle.shouldRestart(),
                .exitCode = lifecycle.getExitCode(),
            };

            dispatchDestroy();

            return result;
        } catch (...) {
            dispatchDestroy();
            throw;
        }
    }
    void Kernel::dispatchSetup()
    {
        const auto next = Next {
            m_plugins,
            0,
            &m_context,
            [](Plugin& plugin, Context& context, const Next& next) {
                plugin.setup(context, next);
            },
            nullptr,
        };

        next();
    }

    void Kernel::dispatchEvent(const WindowEvent& event)
    {
        const auto next = Next {
            m_plugins,
            0,
            &m_context,
            [](Plugin& plugin, Context& context, const Next& next) {
                const WindowEvent& event = *static_cast<const WindowEvent*>(next.getPayload());
                plugin.event(context, next, event);
            },
            &event,
        };

        next();
    }

    void Kernel::dispatchDraw()
    {
        const auto next = Next {
            m_plugins,
            0,
            &m_context,
            [](Plugin& plugin, Context& context, const Next& next) {
                plugin.draw(context, next);
            },
            nullptr,
        };

        next();
    }

    void Kernel::dispatchDestroy()
    {
        const auto next = Next {
            m_plugins,
            0,
            &m_context,
            [](Plugin& plugin, Context& context, const Next& next) {
                plugin.destroy(context, next);
            },
            nullptr,
        };

        next();
    }
} // namespace p5
