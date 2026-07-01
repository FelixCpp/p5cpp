#include "frame_module.hpp"
#include "../../app_context.hpp"
#include "../../timing.hpp"

#include <p5cpp.hpp>

namespace p5cpp
{
    void FrameModule::setup(AppContext& context, Next next)
    {
        info("FrameModule setup");

        fpsCalculationInterval = 1.0f;
        framesPerCalculation = 0;
        lastCalculationTimestamp = std::chrono::steady_clock::now();
        frameStartTimestamp = std::chrono::steady_clock::now();
        lastFrameStart = std::chrono::steady_clock::now();

        context.registerService(&data);

        next();
    }

    void FrameModule::event(AppContext& context, WindowEvent& event, Next next)
    {
        if (event.type == EventType::close) {
            data.closeRequested = true;
        }

        next();
    }

    void FrameModule::draw(AppContext& context, Next next)
    {
        frameStartTimestamp = std::chrono::steady_clock::now();
        const float deltaTime = std::chrono::duration<float>(frameStartTimestamp - lastFrameStart).count();
        lastFrameStart = frameStartTimestamp;

        data.deltaTime = deltaTime;
        data.globalTime += deltaTime;

        if (not data.isPaused) {
            ++framesPerCalculation;
            data.frameCount++;

            const auto now = std::chrono::steady_clock::now();
            const float elapsed = std::chrono::duration<float>(now - lastCalculationTimestamp).count();

            if (elapsed >= fpsCalculationInterval) {
                data.framesPerSecond = static_cast<float>(framesPerCalculation) / elapsed;
                framesPerCalculation = 0;
                lastCalculationTimestamp = now;
            }

            next();
        }

        const int desiredFrameRate = data.targetFrameRate;
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

    void FrameModule::destroy(AppContext& context, Next next)
    {
        next();

        context.unregisterService<FrameData>();
    }
} // namespace p5cpp
