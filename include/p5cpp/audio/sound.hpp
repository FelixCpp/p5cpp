#pragma once

#include <memory>

namespace p5cpp
{
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

        virtual void setVolume(float volume) const = 0;
        virtual float getVolume() const = 0;
        virtual void setPan(float pan) const = 0;
        virtual float getPan() const = 0;
        virtual void setRate(float rate) const = 0;
        virtual float getRate() const = 0;
        virtual void setLoop(bool loop) const = 0;
        virtual bool isLooping() const = 0;
    };

    class Sound
    {
    public:
        Sound();
        Sound(std::unique_ptr<SoundImpl> impl);
        Sound(std::shared_ptr<SoundImpl> impl);

        void play() const;
        void stop() const;
        void pause() const;
        bool isPlaying() const;

        void setVolume(float volume) const;
        float getVolume() const;
        void setPan(float pan) const;
        float getPan() const;
        void setRate(float rate) const;
        float getRate() const;
        void setLoop(bool loop) const;
        bool isLooping() const;

    private:
        std::shared_ptr<SoundImpl> impl;
    };
} // namespace p5cpp
