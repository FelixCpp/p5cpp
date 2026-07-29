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

    namespace detail
    {
        // Opaque - the real miniaudio-backed state, defined only in audio_stream.cpp.
        struct AudioStreamResource;
    }

    class AudioComponent;

    class AudioStream
    {
    public:
        // A default-constructed AudioStream is invalid (no backing resource); all
        // methods on an invalid handle are safe no-ops (getters return zero/default
        // values).
        AudioStream();

        uint32_t sampleRate {0};
        uint32_t channels {0};

        bool isValid() const;
        explicit operator bool() const;

        // Mutating methods are const: they act on the underlying miniaudio resource
        // reached through m_resource, not the handle's own (empty) state - same
        // shallow-const-handle idiom as Sound (see sound.hpp).
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
        friend AudioStream createAudioStreamImpl(AudioComponent& audio, uint32_t sampleRate, uint32_t channels, AudioStreamCallback callback);

        AudioStream(uint32_t sampleRate, uint32_t channels, std::shared_ptr<detail::AudioStreamResource> resource);

        std::shared_ptr<detail::AudioStreamResource> m_resource;
    };
} // namespace p5cpp
