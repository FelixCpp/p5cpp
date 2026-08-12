#include <p5cpp/application/kernel.hpp>
#include <p5cpp/application/lifecycle.hpp>

namespace p5
{
    void Kernel::addPlugin(std::unique_ptr<Plugin> plugin)
    {
        m_plugins.push_back(std::move(plugin));
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
