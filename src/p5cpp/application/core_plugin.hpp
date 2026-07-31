#ifndef P5CPP_CORE_PLUGIN_HPP
#define P5CPP_CORE_PLUGIN_HPP

#include <p5cpp/p5cpp.hpp>

#include <p5cpp/application/lifecycle_orchestrator.hpp>

namespace p5
{
    class CorePlugin : public Plugin
    {
    public:
        CorePlugin();

        void setup(PluginPipeline& pipeline) override;
        void event(PluginPipeline& pipeline, const WindowEvent& event) override;
        void draw(PluginPipeline& pipeline) override;
        void destroy(PluginPipeline& pipeline) override;

    private:
        Lifecycle lifecycle;
    };
} // namespace p5

#endif // !P5CPP_CORE_PLUGIN_HPP
