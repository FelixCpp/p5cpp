#include <p5cpp/application/application.hpp>

namespace p5
{
    class SetupPluginPipeline : public PluginPipeline
    {
    public:
        explicit SetupPluginPipeline(std::span<std::unique_ptr<Plugin>> plugins)
            : index(0), plugins(plugins)
        {
        }

        virtual void next() override
        {
            if (index < plugins.size()) {
                std::unique_ptr<Plugin>& plugin = plugins[index];
                plugin->setup(*this);

                ++index;
            }
        }

    private:
        size_t index;
        std::span<std::unique_ptr<Plugin>> plugins;
    };
} // namespace p5

namespace p5
{
    class EventPluginPipeline : public PluginPipeline
    {
    public:
        explicit EventPluginPipeline(std::span<std::unique_ptr<Plugin>> plugins, const WindowEvent& event)
            : index(0), plugins(plugins), event(event)
        {
        }

        virtual void next() override
        {
            if (index < plugins.size()) {
                std::unique_ptr<Plugin>& plugin = plugins[index];
                plugin->event(*this, event);

                ++index;
            }
        }

    private:
        size_t index;
        std::span<std::unique_ptr<Plugin>> plugins;
        const WindowEvent& event;
    };
} // namespace p5

namespace p5
{
    class DrawPluginPipeline : public PluginPipeline
    {
    public:
        explicit DrawPluginPipeline(std::span<std::unique_ptr<Plugin>> plugins)
            : index(0), plugins(plugins)
        {
        }

        virtual void next() override
        {
            if (index < plugins.size()) {
                std::unique_ptr<Plugin>& plugin = plugins[index];
                plugin->draw(*this);

                ++index;
            }
        }

    private:
        size_t index;
        std::span<std::unique_ptr<Plugin>> plugins;
    };
} // namespace p5

namespace p5
{
    class DestroyPluginPipeline : public PluginPipeline
    {
    public:
        explicit DestroyPluginPipeline(std::span<std::unique_ptr<Plugin>> plugins)
            : index(0), plugins(plugins)
        {
        }

        virtual void next() override
        {
            if (index < plugins.size()) {
                std::unique_ptr<Plugin>& plugin = plugins[index];
                plugin->destroy(*this);

                ++index;
            }
        }

    private:
        size_t index;
        std::span<std::unique_ptr<Plugin>> plugins;
    };
} // namespace p5

namespace p5
{
    void Application::add(std::unique_ptr<Plugin> plugin)
    {
        plugins.emplace_back(std::move(plugin));
    }

    void Application::run()
    {
        {
            SetupPluginPipeline setupPipeline(plugins);
            setupPipeline.next();
        }

        {
            DrawPluginPipeline drawPipeline(plugins);
            drawPipeline.next();
        }

        {
            DestroyPluginPipeline destroyPipeline(plugins);
            destroyPipeline.next();
        }
    }

    void Application::dispatch(const WindowEvent& event)
    {
        EventPluginPipeline eventPipeline(plugins, event);
        eventPipeline.next();
    }
} // namespace p5
