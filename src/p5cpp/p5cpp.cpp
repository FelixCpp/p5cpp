#include <p5cpp/p5cpp.hpp>
#include <p5cpp/application/lifecycle.hpp>
#include <p5cpp/application/lifecycle_plugin.hpp>

#include <vector>

namespace p5
{
    struct RunResult
    {
        bool shouldRestart;
        int exitCode;
    };

    class Kernel
    {
    public:
        Kernel()
        {
        }

        void addPlugin(std::unique_ptr<Plugin> plugin)
        {
            m_plugins.push_back(std::move(plugin));
        }

        RunResult run()
        {
            dispatchSetup();

            Lifecycle& lifecycle = m_context.require<Lifecycle>();

            while (not lifecycle.shouldClose()) {
                dispatchDraw();
            }

            RunResult result {
                .shouldRestart = lifecycle.shouldRestart(),
                .exitCode = lifecycle.getExitCode(),
            };

            dispatchDestroy();

            return result;
        }

    private:
        void dispatchSetup()
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

        void dispatchEvent(const WindowEvent& event)
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

        void dispatchDraw()
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

        void dispatchDestroy()
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

    private:
        std::vector<std::unique_ptr<Plugin>> m_plugins;
        Context m_context;
    };
} // namespace p5

int main()
{
    using namespace p5;

    RunResult result;

    do {
        const auto kernel = std::make_unique<Kernel>();

        kernel->addPlugin(std::make_unique<LifecyclePlugin>());
        result = kernel->run();
    } while (result.shouldRestart);

    return result.exitCode;
}
