#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <vector>

namespace p5cpp
{
    // Fully decoded PCM data: interleaved 32-bit float samples.
    struct AudioSamples
    {
        std::vector<float> samples;
        uint32_t sampleRate {0};
        uint32_t channels {0};

        bool isValid() const { return channels != 0 && sampleRate != 0; }
        uint64_t frameCount() const { return channels == 0 ? 0 : samples.size() / channels; }
        float duration() const { return sampleRate == 0 ? 0.0f : static_cast<float>(frameCount()) / static_cast<float>(sampleRate); }
    };

    // Invoked on the AUDIO THREAD whenever the engine needs more samples:
    // fill `frames` (interleaved, pre-zeroed, frames.size() = frameCount * channels).
    // Do not call any p5cpp functions or allocate inside the callback.
    using AudioStreamCallback = std::function<void(std::span<float> frames, uint32_t channels)>;

    // Same shallow-const-handle idiom as SoundImpl (see sound.hpp).
    struct AudioStreamImpl
    {
        virtual ~AudioStreamImpl() = default;

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
    };

    class AudioStream
    {
    public:
        AudioStream();
        AudioStream(std::unique_ptr<AudioStreamImpl> impl);
        AudioStream(std::shared_ptr<AudioStreamImpl> impl);

        // An AudioStream is invalid when creation failed; all methods on an
        // invalid handle are safe no-ops (getters return zero/default values).
        bool isValid() const;
        explicit operator bool() const;

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

    private:
        std::shared_ptr<AudioStreamImpl> impl;
    };
} // namespace p5cpp
