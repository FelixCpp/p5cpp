#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <span>

namespace p5cpp
{
    enum class SoundStatus
    {
        stopped,
        paused,
        playing,
    };

    class AudioComponent;

    namespace detail
    {
        // Opaque - the real miniaudio-backed state, defined only in sound.cpp.
        struct SoundResource;
    }

    class Sound
    {
    public:
        // A default-constructed Sound is invalid (no backing resource); all methods
        // on an invalid handle are safe no-ops (getters return zero/default values).
        Sound();

        // Playback-side format. Sounds loaded from a file (loadSound/loadMusic) are
        // converted to the engine's playback format, so these may differ from the
        // file's native values — they stay self-consistent (frameCount() / sampleRate
        // == duration()), which is what matters for seek/loop-point math. Use
        // loadAudioSamples() to inspect a file's native format.
        uint32_t sampleRate {0};
        uint32_t channels {0};

        bool isValid() const;
        explicit operator bool() const;

        // Mutating methods are const: they act on the underlying audio-engine resource
        // reached through m_resource, not on the handle's own (empty) state — the same
        // shallow-const-handle idiom Framebuffer/Font already use.
        void play() const;
        void stop() const;
        void pause() const;
        bool isPlaying() const;
        SoundStatus status() const;

        void setVolume(float volume) const;
        float getVolume() const;
        void setPan(float pan) const;
        float getPan() const;
        void setRate(float rate) const;
        float getRate() const;
        void setLoop(bool loop) const;
        bool isLooping() const;
        void setLoopPoints(float startSeconds, float endSeconds) const;

        void seek(float seconds) const;
        float currentTime() const;
        float duration() const;

        // Fades apply a volume multiplier on top of setVolume(); pass -1.0f as
        // fromVolume to fade from the current fade volume. fadeOut() only turns
        // the sound silent — it does not stop playback.
        void setFade(float fromVolume, float toVolume, float milliseconds) const;
        void fadeIn(float milliseconds) const;
        void fadeOut(float milliseconds) const;

        // Not promoted to a field like sampleRate/channels: for streamed music
        // (loadMusic()), some formats can't report their length immediately after
        // load, so this stays a live, on-demand query rather than a value cached
        // once at construction.
        uint64_t frameCount() const;

        // The callback is invoked on the main thread (during the frame update)
        // after the sound reaches its natural end. Looping sounds never end.
        void onEnded(std::function<void()> callback) const;

        // Creates an independently playable sound sharing the same decoded data
        // (like raylib's LoadSoundAlias). Not supported for streamed music —
        // returns an invalid Sound in that case.
        Sound clone() const;

    private:
        friend class detail::SoundResource;
        friend Sound loadSoundImpl(AudioComponent& audio, const std::filesystem::path& soundFilePath, bool stream);
        friend Sound loadSoundImpl(AudioComponent& audio, std::span<const uint8_t> soundData);
        friend Sound createSoundImpl(AudioComponent& audio, std::span<const float> samples, uint32_t sampleRate, uint32_t channels);

        Sound(uint32_t sampleRate, uint32_t channels, std::shared_ptr<detail::SoundResource> resource);

        std::shared_ptr<detail::SoundResource> m_resource;
    };
} // namespace p5cpp
