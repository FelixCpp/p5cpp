#include <p5cpp/p5cpp.hpp>
#include <p5cpp_audio/p5cpp_audio.hpp>

#include <atomic>
#include <cmath>
#include <vector>

using namespace p5;
using namespace p5::audio;

struct AudioVisualizer : Sketch
{
    Sound sound = loadSoundFromFile("sound.mp3");
    Sound alias = createSoundAlias(sound);

    MixedAudioProcessorHandle levelProcessor;
    std::atomic<float> level {0.0f};

    SoundProcessorHandle soundLevelProcessor;
    std::atomic<float> soundLevel {0.0f};

    static float rmsOf(const std::span<const float> frames)
    {
        float sumSquares = 0.0f;
        for (const float sample : frames) {
            sumSquares += sample * sample;
        }

        return frames.empty() ? 0.0f : std::sqrt(sumSquares / static_cast<float>(frames.size()));
    }

    void setup() override
    {
        setWindowSize(800, 600);

        setSoundVolume(sound, 1.0f);
        setSoundVolume(alias, 0.4f);
        setSoundLoop(sound, true);

        levelProcessor = attachMixedAudioProcessor([this](const std::span<float> frames, uint32_t) {
            level.store(rmsOf(frames), std::memory_order_relaxed);
        });

        soundLevelProcessor = attachSoundProcessor(sound, [this](const std::span<float> frames, uint32_t) {
            soundLevel.store(rmsOf(frames), std::memory_order_relaxed);
        });
    }

    void event(const WindowEvent& event) override
    {
    }

    void draw() override
    {
        background(rgba(20, 20, 30));

        if (isKeyPressed(Key::A)) {
            playSound(sound);
        }

        if (isKeyPressed(Key::B)) {
            playSound(alias);
        }

        const uint2 windowSize = getWindowSize();
        const float w = static_cast<float>(windowSize.x);
        const float h = static_cast<float>(windowSize.y);

        const float mixedHeight = std::clamp(level.load(std::memory_order_relaxed) * 4.0f, 0.0f, 1.0f) * h;
        const float soundHeight = std::clamp(soundLevel.load(std::memory_order_relaxed) * 4.0f, 0.0f, 1.0f) * h;

        noStroke();
        fill(rgba(80, 200, 255));
        rect(w * 0.5f - 100.0f, h - mixedHeight, 80.0f, mixedHeight);

        fill(rgba(255, 140, 80));
        rect(w * 0.5f + 20.0f, h - soundHeight, 80.0f, soundHeight);
    }

    ~AudioVisualizer() override
    {
        detachSoundProcessor(sound, soundLevelProcessor);
        detachMixedAudioProcessor(levelProcessor);
    }
};

SketchSpec p5::createSpec()
{
    return {
        .plugins = [] {
            std::vector<std::unique_ptr<Plugin>> plugins;
            plugins.push_back(createAudioPlugin());
            return plugins;
        },
        .sketch = [] {
            return std::make_unique<AudioVisualizer>();
        },
    };
}
