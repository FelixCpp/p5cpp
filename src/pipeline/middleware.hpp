#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "../services/window.hpp"
#include "p5cpp.hpp"

namespace p5cpp
{
    struct AppContext
    {
        Window* window;

        bool shouldClose;
        float deltaTime;
        uint64_t frameCount;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct InvocationContext
    {
        virtual ~InvocationContext() = default;

        virtual void next() = 0;
        virtual void abort() = 0;
    };
} // namespace p5cpp

namespace p5cpp
{
    // This interface allows you to implement your own middleware for the p5cpp framework.
    // It provides hooks for various events in the application lifecycle, such as setup, draw, and event handling.
    struct Middleware
    {
        virtual ~Middleware() = default;

        virtual void setup(AppContext& context, InvocationContext& pipeline) {}
        virtual void event(AppContext& context, WindowEvent& event, InvocationContext& pipeline) {}
        virtual void draw(AppContext& context, InvocationContext& pipeline) {}
        virtual void destroy(AppContext& context, InvocationContext& pipeline) {}
    };
} // namespace p5cpp

namespace p5cpp
{
    class MiddlewarePipeline
    {
    public:
        void run();

        void add(std::unique_ptr<Middleware> middleware);

    private:
        struct PipelineInvocationContext : public InvocationContext
        {
            explicit PipelineInvocationContext(MiddlewarePipeline& pipeline, size_t currentIndex);

            void next() override;
            void abort() override;

            MiddlewarePipeline& pipeline;
            size_t currentIndex;
        };

        AppContext context;
        std::vector<std::unique_ptr<Middleware>> middlewares;
    };
} // namespace p5cpp
