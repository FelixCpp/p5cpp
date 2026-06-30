#include "engine.hpp"
#include "app_context.hpp"

namespace p5cpp
{
    class AppEngine : public Engine
    {
    public:
        void addModule(std::unique_ptr<Module> module) override
        {
            modules.push_back(std::move(module));
        }

        void dispatch(const WindowEvent& event) override
        {
            WindowEvent copy = event;
            eventModules(copy);
        }

        void run() override
        {
            context.engine = this;

            setupModules();

            while (not context.lifecycleInfo.closeRequested) {
                drawModules();
            }

            destroyModules();
        }

        AppContext& getContext() override
        {
            return context;
        }

    private:
        void setupModules(size_t i = 0)
        {
            if (i >= modules.size()) {
                return;
            }

            modules[i]->setup(context, [this, i]() {
                setupModules(i + 1);
            });
        }

        void drawModules(size_t i = 0)
        {
            if (i >= modules.size()) {
                return;
            }

            modules[i]->draw(context, [this, i]() {
                drawModules(i + 1);
            });
        }

        void destroyModules(size_t i = 0)
        {
            if (i >= modules.size()) {
                return;
            }

            modules[i]->destroy(context, [this, i]() {
                destroyModules(i + 1);
            });
        }

        void eventModules(WindowEvent& event, size_t i = 0)
        {
            if (i >= modules.size()) {
                return;
            }

            modules[i]->event(context, event, [this, i, &event]() {
                eventModules(event, i + 1);
            });
        }

        AppContext context;
        std::vector<std::unique_ptr<Module>> modules;
    };
} // namespace p5cpp

namespace p5cpp
{
    std::unique_ptr<Engine> Engine::create()
    {
        return std::make_unique<AppEngine>();
    }
} // namespace p5cpp
