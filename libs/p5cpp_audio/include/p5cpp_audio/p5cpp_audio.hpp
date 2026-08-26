#pragma once

#include <p5cpp/p5cpp.hpp>

namespace p5::audio
{
    struct SoundResource;
    struct Sound
    {
        std::shared_ptr<SoundResource> resource;
    };

    enum class PlaybackState
    {
        Stopped, // never played, or explicitly stopped via stopSound() -- position is at the start
        Playing, // currently playing, not stopped or paused
        Paused,  // paused via pauseSound() -- position is preserved, ready to continue via resumeSound()
    };

    Sound loadSoundFromFile(const std::filesystem::path& filepath);
    Sound loadSoundFromMemory(std::span<const uint8_t> data);

    void playSound(const Sound& sound);
    void pauseSound(const Sound& sound);
    void resumeSound(const Sound& sound);
    void stopSound(const Sound& sound);
    bool isSoundPlaying(const Sound& sound);
    PlaybackState getSoundPlaybackState(const Sound& sound);
    bool isSoundValid(const Sound& sound);

    void setSoundVolume(const Sound& sound, float volume);
    void setSoundPitch(const Sound& sound, float pitch);
    void setSoundPan(const Sound& sound, float pan);

    void setSoundLoop(const Sound& sound, bool loop);
    bool isSoundLooping(const Sound& sound);

    void seekSound(const Sound& sound, float seconds);
    float getSoundTimePlayed(const Sound& sound);
    float getSoundTimeLength(const Sound& sound);

    void setMasterVolume(float volume);
    float getMasterVolume();

    void playSoundOverlapped(const Sound& sound);
    Sound createSoundAlias(const Sound& sound);

    using MixedAudioProcessor = std::function<void(std::span<float> frames, uint32_t channels)>;
    struct MixedAudioProcessorHandle
    {
        uint64_t id = 0;
    };

    MixedAudioProcessorHandle attachMixedAudioProcessor(MixedAudioProcessor processor);
    void detachMixedAudioProcessor(MixedAudioProcessorHandle handle);

    using SoundProcessor = std::function<void(std::span<float> frames, uint32_t channels)>;
    struct SoundProcessorHandle
    {
        uint64_t id = 0;
    };

    SoundProcessorHandle attachSoundProcessor(const Sound& sound, SoundProcessor processor);
    void detachSoundProcessor(const Sound& sound, SoundProcessorHandle handle);
} // namespace p5::audio

namespace p5::audio
{
    std::unique_ptr<Plugin> createAudioPlugin();
} // namespace p5::audio
