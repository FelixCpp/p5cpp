#include <p5cpp/application/engine.hpp>
#include <p5cpp/application/app_context.hpp>
#include <p5cpp/audio/audio_component.hpp>

#include <span>

namespace p5cpp
{
    extern std::unique_ptr<Engine> engine;
}

namespace p5cpp
{
    AudioComponent& getAudioComponent()
    {
        static AudioComponent* s_audioComponent = nullptr;
        static Engine* s_engine = nullptr;
        if (s_engine != engine.get()) {
            s_engine = engine.get();
            s_audioComponent = &engine->getContext().require<AudioComponent>();
        }
        return *s_audioComponent;
    }
} // namespace p5cpp

namespace p5cpp
{
    void playSoundMulti(const Sound& sound) { getAudioComponent().playMulti(sound); }

    void masterVolume(float volume) { getAudioComponent().setMasterVolume(volume); }
    float getMasterVolume() { return getAudioComponent().getMasterVolume(); }

    float getAudioAmplitude() { return getAudioComponent().getAmplitude(); }
    std::span<const float> getAudioWaveform() { return getAudioComponent().getWaveform(); }
} // namespace p5cpp
