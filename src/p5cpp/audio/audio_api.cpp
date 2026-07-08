#include <p5cpp/application/engine.hpp>
#include <p5cpp/application/app_context.hpp>
#include <p5cpp/audio/audio_component.hpp>
#include <p5cpp/audio/sound_impl.hpp>

#include <cstdint>
#include <filesystem>
#include <span>

namespace p5cpp
{
    extern std::unique_ptr<Engine> engine;
}

namespace p5cpp
{
    AudioComponent& getAudioComponent()
    {
        return engine->getContext().require<AudioComponent>();
    }
} // namespace p5cpp

namespace p5cpp
{
    Sound loadSound(const std::filesystem::path& soundFilePath) { return Sound(loadSoundImpl(getAudioComponent().getEngine(), soundFilePath)); }
    Sound loadSound(std::span<const uint8_t> soundData) { return Sound(loadSoundImpl(getAudioComponent().getEngine(), soundData)); }

    void playSound(const Sound& sound) { sound.play(); }
    void stopSound(const Sound& sound) { sound.stop(); }
    void pauseSound(const Sound& sound) { sound.pause(); }
    bool isPlaying(const Sound& sound) { return sound.isPlaying(); }

    void masterVolume(float volume) { getAudioComponent().setMasterVolume(volume); }
    float getMasterVolume() { return getAudioComponent().getMasterVolume(); }

    float getAudioAmplitude() { return getAudioComponent().getAmplitude(); }
    std::span<const float> getAudioWaveform() { return getAudioComponent().getWaveform(); }
} // namespace p5cpp
