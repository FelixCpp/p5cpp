#include <p5cpp/application/core_plugin.hpp>

namespace p5
{
    CorePlugin::CorePlugin()
        : lifecycle {
              .shouldClose = false,
              .exitCode = 0,
          }
    {
    }

    void CorePlugin::setup(PluginPipeline& pipeline)
    {
        addDependency(&lifecycle);
        Lifecycle& lifecycle = getDependency<Lifecycle>();

        pipeline.next();
    }

    void CorePlugin::event(PluginPipeline& pipeline, const WindowEvent& event)
    {
        pipeline.next();
    }

    void CorePlugin::draw(PluginPipeline& pipeline)
    {
        while (not lifecycle.shouldClose) {
            pipeline.next();
        }
    }

    void CorePlugin::destroy(PluginPipeline& pipeline)
    {
        pipeline.next();
    }
} // namespace p5
