#ifndef P5CPP_LIFECYCLE_ORCHESTRATOR_HPP
#define P5CPP_LIFECYCLE_ORCHESTRATOR_HPP

namespace p5
{
    struct Lifecycle
    {
        bool shouldClose;
        int exitCode;
    };
} // namespace p5

#endif // !P5CPP_LIFECYCLE_ORCHESTRATOR_HPP
