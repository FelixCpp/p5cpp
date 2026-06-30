#include "lifecycle_module.hpp"
#include "../app_context.hpp"
#include "../timing.hpp"

namespace p5cpp
{
    void LifecycleModule::setup(AppContext& context, Next next)
    {
        info("LifecycleModule setup");

        fpsCalculationInterval = 1.0f;
        framesPerCalculation = 0;
        lastCalculationTimestamp = std::chrono::steady_clock::now();
        frameStartTimestamp = std::chrono::steady_clock::now();
        lastFrameStart = std::chrono::steady_clock::now();

        next();
    }

    void LifecycleModule::event(AppContext& context, WindowEvent& event, Next next)
    {
        if (event.type == EventType::close) {
            context.lifecycleInfo.closeRequested = true;
        }

        next();
    }

    void LifecycleModule::draw(AppContext& context, Next next)
    {
        frameStartTimestamp = std::chrono::steady_clock::now();
        const float deltaTime = std::chrono::duration<float>(frameStartTimestamp - lastFrameStart).count();
        lastFrameStart = frameStartTimestamp;

        context.frameInfo.deltaTime = deltaTime;
        context.frameInfo.globalTime += deltaTime;

        if (not context.lifecycleInfo.isPaused) {
            ++framesPerCalculation;
            context.frameInfo.frameCount++;

            const auto now = std::chrono::steady_clock::now();
            const float elapsed = std::chrono::duration<float>(now - lastCalculationTimestamp).count();

            if (elapsed >= fpsCalculationInterval) {
                context.frameInfo.framesPerSecond = static_cast<float>(framesPerCalculation) / elapsed;
                framesPerCalculation = 0;
                lastCalculationTimestamp = now;
            }

            next();
        }

        const int desiredFrameRate = context.frameInfo.targetFrameRate;
        if (desiredFrameRate > 0) {
            const auto frameEndTimestamp = std::chrono::steady_clock::now();
            const float frameDuration = std::chrono::duration<float>(frameEndTimestamp - frameStartTimestamp).count();
            const float targetFrameTime = 1.0f / static_cast<float>(desiredFrameRate);
            const float sleepDuration = targetFrameTime - frameDuration;

            if (sleepDuration > 0.0f) {
                const auto sleepUntilTimestamp = frameEndTimestamp + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<float>(sleepDuration));
                precise_sleep_until(sleepUntilTimestamp);
            }
        }
    }
} // namespace p5cpp
