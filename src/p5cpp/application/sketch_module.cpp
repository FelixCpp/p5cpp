#include <p5cpp/application/sketch_module.hpp>
#include <p5cpp/application/logging.hpp>
#include <p5cpp/application/app_context.hpp>

namespace p5cpp
{
    void SketchModule::addModuleBefore(std::unique_ptr<Module> module)
    {
        m_preModules.push_back(std::move(module));
    }

    void SketchModule::addModuleAfter(std::unique_ptr<Module> module)
    {
        m_postModules.push_back(std::move(module));
    }

    void SketchModule::setup(AppContext& context, Next next)
    {
        info("SketchModule setup");

        Engine& engine = context.require<Engine>();

        sketch = createSketch();
        sketch->plugins(engine);
        sketch->registerModules(*this);

        context.registerService(sketch.get());

        buildDrawChain(context);

        setupPreModules(context, next);
    }

    void SketchModule::setupPreModules(AppContext& context, Next next, size_t i)
    {
        if (i >= m_preModules.size()) {
            sketch->setup();
            setupPostModules(context, next);
            return;
        }

        m_preModules[i]->setup(context, [this, &context, next, i]() {
            setupPreModules(context, next, i + 1);
        });
    }

    void SketchModule::setupPostModules(AppContext& context, Next next, size_t i)
    {
        if (i >= m_postModules.size()) {
            next();
            return;
        }

        m_postModules[i]->setup(context, [this, &context, next, i]() {
            setupPostModules(context, next, i + 1);
        });
    }

    void SketchModule::event(AppContext& context, WindowEvent& event, Next next)
    {
        eventPreModules(context, event, next);
    }

    void SketchModule::eventPreModules(AppContext& context, WindowEvent& event, Next next, size_t i)
    {
        if (i >= m_preModules.size()) {
            sketch->event(event);
            eventPostModules(context, event, next);
            return;
        }

        m_preModules[i]->event(context, event, [this, &context, &event, next, i]() {
            eventPreModules(context, event, next, i + 1);
        });
    }

    void SketchModule::eventPostModules(AppContext& context, WindowEvent& event, Next next, size_t i)
    {
        if (i >= m_postModules.size()) {
            next();
            return;
        }

        m_postModules[i]->event(context, event, [this, &context, &event, next, i]() {
            eventPostModules(context, event, next, i + 1);
        });
    }

    void SketchModule::draw(AppContext& context, Next next)
    {
        m_drawNext = std::move(next);
        m_drawChain();
    }

    void SketchModule::buildDrawChain(AppContext& context)
    {
        // Terminal step: invoke whatever outer continuation the current frame's
        // draw() call was given (read live, not captured, since it changes per call).
        Next chain = [this]() {
            m_drawNext();
        };

        for (size_t i = m_postModules.size(); i-- > 0; ) {
            Module* module = m_postModules[i].get();
            Next inner = std::move(chain);
            chain = [this, &context, module, inner]() {
                module->draw(context, inner);
            };
        }

        Next afterPreModules = [this, postChain = std::move(chain)]() {
            sketch->draw();
            postChain();
        };

        Next fullChain = std::move(afterPreModules);
        for (size_t i = m_preModules.size(); i-- > 0; ) {
            Module* module = m_preModules[i].get();
            Next inner = std::move(fullChain);
            fullChain = [this, &context, module, inner]() {
                module->draw(context, inner);
            };
        }

        m_drawChain = std::move(fullChain);
    }

    void SketchModule::destroy(AppContext& context, Next next)
    {
        next();

        context.unregisterService<Sketch>();

        destroyPreModules(context);
    }

    void SketchModule::destroyPreModules(AppContext& context, size_t i)
    {
        if (i >= m_preModules.size()) {
            sketch->destroy();
            destroyPostModules(context);
            return;
        }

        m_preModules[i]->destroy(context, [this, &context, i]() {
            destroyPreModules(context, i + 1);
        });
    }

    void SketchModule::destroyPostModules(AppContext& context, size_t i)
    {
        if (i >= m_postModules.size()) {
            sketch.reset();
            return;
        }

        m_postModules[i]->destroy(context, [this, &context, i]() {
            destroyPostModules(context, i + 1);
        });
    }
} // namespace p5cpp
