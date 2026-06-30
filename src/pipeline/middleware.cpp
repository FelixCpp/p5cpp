#include "middleware.hpp"

namespace p5cpp
{
    void MiddlewarePipeline::run()
    {
        {
            PipelineInvocationContext invocationContext(*this, 0);
            invocationContext.next();
        }
    }

    MiddlewarePipeline::PipelineInvocationContext::PipelineInvocationContext(MiddlewarePipeline& pipeline, size_t currentIndex)
        : pipeline(pipeline), currentIndex(currentIndex)
    {
    }

    void MiddlewarePipeline::PipelineInvocationContext::next()
    {
        if (currentIndex < pipeline.middlewares.size()) {
            Middleware* middleware = pipeline.middlewares[currentIndex].get();
            PipelineInvocationContext nextContext(pipeline, currentIndex + 1);
            middleware->setup(pipeline.context, nextContext);
        }
    }

    void MiddlewarePipeline::PipelineInvocationContext::abort()
    {
    }
} // namespace p5cpp
