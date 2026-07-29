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
    Sound loadSound(const std::filesystem::path& soundFilePath) { return loadSoundImpl(getAudioComponent(), soundFilePath, false); }
    Sound loadSound(std::span<const uint8_t> soundData) { return loadSoundImpl(getAudioComponent(), soundData); }
    Sound loadMusic(const std::filesystem::path& musicFilePath) { return loadSoundImpl(getAudioComponent(), musicFilePath, true); }
    Sound createSound(std::span<const float> samples, uint32_t sampleRate, uint32_t channels) { return createSoundImpl(getAudioComponent(), samples, sampleRate, channels); }

    AudioSamples loadAudioSamples(const std::filesystem::path& soundFilePath) { return decodeAudioSamples(soundFilePath); }
    AudioSamples loadAudioSamples(std::span<const uint8_t> soundData) { return decodeAudioSamples(soundData); }

    AudioStream createAudioStream(uint32_t sampleRate, uint32_t channels, AudioStreamCallback callback) { return createAudioStreamImpl(getAudioComponent(), sampleRate, channels, std::move(callback)); }

    void playSound(const Sound& sound) { sound.play(); }
    void stopSound(const Sound& sound) { sound.stop(); }
    void pauseSound(const Sound& sound) { sound.pause(); }
    bool isPlaying(const Sound& sound) { return sound.isPlaying(); }
    void seekSound(const Sound& sound, float seconds) { sound.seek(seconds); }
    void playSoundMulti(const Sound& sound) { getAudioComponent().playMulti(sound); }

    void masterVolume(float volume) { getAudioComponent().setMasterVolume(volume); }
    float getMasterVolume() { return getAudioComponent().getMasterVolume(); }

    float getAudioAmplitude() { return getAudioComponent().getAmplitude(); }
    std::span<const float> getAudioWaveform() { return getAudioComponent().getWaveform(); }
} // namespace p5cpp
