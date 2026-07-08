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
        drawPreModules(context, next);
    }

    void SketchModule::drawPreModules(AppContext& context, Next next, size_t i)
    {
        if (i >= m_preModules.size()) {
            sketch->draw();
            drawPostModules(context, next);
            return;
        }

        m_preModules[i]->draw(context, [this, &context, next, i]() {
            drawPreModules(context, next, i + 1);
        });
    }

    void SketchModule::drawPostModules(AppContext& context, Next next, size_t i)
    {
        if (i >= m_postModules.size()) {
            next();
            return;
        }

        m_postModules[i]->draw(context, [this, &context, next, i]() {
            drawPostModules(context, next, i + 1);
        });
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
