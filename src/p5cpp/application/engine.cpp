#include <p5cpp/application/engine.hpp>
#include <p5cpp/application/window_event.hpp>
#include <p5cpp/application/frame_component.hpp>

#include <p5cpp/application/app_context.hpp>

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
            context.registerService<Engine>(this);

            setupModules();
            buildDrawChain();

            FrameComponent& frameData = context.require<FrameComponent>();
            while (not frameData.isCloseRequested()) {
                m_drawChain();
            }

            m_restartRequested = frameData.isRestartRequested();
            destroyModules();
        }

        bool wasRestartRequested() const override
        {
            return m_restartRequested;
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

        // Builds the fixed draw dispatch chain once (the module list never changes
        // after setup), instead of reconstructing a std::function closure per module
        // on every single frame. Folding from the last module backward means each
        // closure captures the already-built Next for the rest of the chain by
        // value, producing the exact same onion call structure as the recursive
        // version — modules can still act before *and* after calling next().
        void buildDrawChain()
        {
            Next chain = []() {};
            for (size_t i = modules.size(); i-- > 0; ) {
                Module* module = modules[i].get();
                Next inner = std::move(chain);
                chain = [this, module, inner]() {
                    module->draw(context, inner);
                };
            }
            m_drawChain = std::move(chain);
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
        Next m_drawChain;
        bool m_restartRequested = false;
    };
} // namespace p5cpp

namespace p5cpp
{
    std::unique_ptr<Engine> Engine::create()
    {
        return std::make_unique<AppEngine>();
    }
} // namespace p5cpp
