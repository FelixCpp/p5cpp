#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace p5cpp
{
    enum class SoundStatus
    {
        stopped,
        paused,
        playing,
    };

    // Mutating methods are const: they act on the underlying audio-engine resource
    // reached through `impl`/`m_sound`, not on the handle's own (empty) state —
    // the same shallow-const-handle idiom Framebuffer/Font already use.
    struct SoundImpl
    {
        virtual ~SoundImpl() = default;

        virtual void play() const = 0;
        virtual void stop() const = 0;
        virtual void pause() const = 0;
        virtual bool isPlaying() const = 0;
        virtual SoundStatus status() const = 0;

        virtual void setVolume(float volume) const = 0;
        virtual float getVolume() const = 0;
        virtual void setPan(float pan) const = 0;
        virtual float getPan() const = 0;
        virtual void setRate(float rate) const = 0;
        virtual float getRate() const = 0;
        virtual void setLoop(bool loop) const = 0;
        virtual bool isLooping() const = 0;
        virtual void setLoopPoints(float startSeconds, float endSeconds) const = 0;

        virtual void seek(float seconds) const = 0;
        virtual float currentTime() const = 0;
        virtual float duration() const = 0;

        virtual void setFade(float fromVolume, float toVolume, float milliseconds) const = 0;

        virtual uint32_t sampleRate() const = 0;
        virtual uint32_t channels() const = 0;
        virtual uint64_t frameCount() const = 0;

        virtual void onEnded(std::function<void()> callback) const = 0;
        virtual std::shared_ptr<SoundImpl> clone() const = 0;
    };

    class Sound
    {
    public:
        Sound();
        Sound(std::unique_ptr<SoundImpl> impl);
        Sound(std::shared_ptr<SoundImpl> impl);

        // A Sound is invalid when loading failed; all methods on an invalid
        // handle are safe no-ops (getters return zero/default values).
        bool isValid() const;
        explicit operator bool() const;

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

        // Playback-side format. Sounds loaded from a file (loadSound/loadMusic)
        // are converted to the engine's playback format, so these may differ from
        // the file's native values — they stay self-consistent (frameCount /
        // sampleRate == duration), which is what matters for seek/loop-point
        // math. Use loadAudioSamples() to inspect a file's native format.
        uint32_t sampleRate() const;
        uint32_t channels() const;
        uint64_t frameCount() const;

        // The callback is invoked on the main thread (during the frame update)
        // after the sound reaches its natural end. Looping sounds never end.
        void onEnded(std::function<void()> callback) const;

        // Creates an independently playable sound sharing the same decoded data
        // (like raylib's LoadSoundAlias). Not supported for streamed music —
        // returns an invalid Sound in that case.
        Sound clone() const;

    private:
        std::shared_ptr<SoundImpl> impl;
    };
} // namespace p5cpp
