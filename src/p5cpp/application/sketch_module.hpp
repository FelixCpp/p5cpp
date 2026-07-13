#pragma once

#include <p5cpp/application/sketch.hpp>
#include <p5cpp/application/module.hpp>

#include <vector>

namespace p5cpp
{
    class SketchModule : public Module, private ModuleRegistrar
    {
    public:
        void setup(AppContext& context, Next next) override;
        void event(AppContext& context, WindowEvent& event, Next next) override;
        void draw(AppContext& context, Next next) override;
        void destroy(AppContext& context, Next next) override;

    private:
        void addModuleBefore(std::unique_ptr<Module> module) override;
        void addModuleAfter(std::unique_ptr<Module> module) override;

        void setupPreModules(AppContext& context, Next next, size_t i = 0);
        void setupPostModules(AppContext& context, Next next, size_t i = 0);

        void buildDrawChain(AppContext& context);

        void eventPreModules(AppContext& context, WindowEvent& event, Next next, size_t i = 0);
        void eventPostModules(AppContext& context, WindowEvent& event, Next next, size_t i = 0);

        void destroyPreModules(AppContext& context, size_t i = 0);
        void destroyPostModules(AppContext& context, size_t i = 0);

        std::unique_ptr<Sketch> sketch;
        std::vector<std::unique_ptr<Module>> m_preModules;
        std::vector<std::unique_ptr<Module>> m_postModules;

        // Fixed pre-modules -> sketch->draw() -> post-modules chain, built once
        // (see buildDrawChain) instead of reconstructed every frame. m_drawNext
        // holds the current frame's outer continuation, read by the chain's
        // terminal step at invocation time.
        Next m_drawChain;
        Next m_drawNext;
    };
} // namespace p5cpp
