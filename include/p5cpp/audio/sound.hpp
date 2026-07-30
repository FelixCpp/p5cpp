#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <span>

namespace p5cpp
{
    enum class SoundStatus {
        stopped,
        paused,
        playing,
    };

    namespace detail
    {
        struct SoundResource;
    }

    struct Sound;

    Sound loadSound(const std::filesystem::path& soundFilePath);
    Sound loadSound(std::span<const uint8_t> soundData);
    Sound loadMusic(const std::filesystem::path& musicFilePath);
    Sound createSound(std::span<const float> samples, uint32_t sampleRate, uint32_t channels);

    struct Sound
    {
        uint32_t sampleRate = 0;
        uint32_t channels = 0;
        std::shared_ptr<detail::SoundResource> resource;
    };

    bool isSoundValid(const Sound& sound);

    void playSound(const Sound& sound);
    void stopSound(const Sound& sound);
    void pauseSound(const Sound& sound);
    bool isPlaying(const Sound& sound);
    SoundStatus getSoundStatus(const Sound& sound);

    void setSoundVolume(const Sound& sound, float volume);
    float getSoundVolume(const Sound& sound);
    void setSoundPan(const Sound& sound, float pan);
    float getSoundPan(const Sound& sound);
    void setSoundRate(const Sound& sound, float rate);
    float getSoundRate(const Sound& sound);
    void setSoundLoop(const Sound& sound, bool loop);
    bool isSoundLooping(const Sound& sound);
    void setSoundLoopPoints(const Sound& sound, float startSeconds, float endSeconds);

    void seekSound(const Sound& sound, float seconds);
    float getSoundCurrentTime(const Sound& sound);
    float getSoundDuration(const Sound& sound);

    // Fades apply a volume multiplier on top of setSoundVolume(); pass -1.0f as
    // fromVolume to fade from the current fade volume. fadeOutSound() only turns
    // the sound silent — it does not stop playback.
    void setSoundFade(const Sound& sound, float fromVolume, float toVolume, float milliseconds);
    void fadeInSound(const Sound& sound, float milliseconds);
    void fadeOutSound(const Sound& sound, float milliseconds);

    uint64_t getSoundFrameCount(const Sound& sound);

    // The callback is invoked on the main thread (during the frame update)
    // after the sound reaches its natural end. Looping sounds never end.
    void onSoundEnded(const Sound& sound, std::function<void()> callback);

    // Creates an independently playable sound sharing the same decoded data
    // (like raylib's LoadSoundAlias). Not supported for streamed music —
    // returns an invalid Sound in that case.
    Sound cloneSound(const Sound& sound);
} // namespace p5cpp
