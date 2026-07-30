#pragma once

#include <cstdint>
#include <filesystem>
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

    // Fully decodes an audio file into raw PCM samples for analysis/processing.
    // Returns an invalid AudioSamples (see AudioSamples::isValid()) on failure.
    AudioSamples loadAudioSamples(const std::filesystem::path& soundFilePath);
    AudioSamples loadAudioSamples(std::span<const uint8_t> soundData);

    // Invoked on the AUDIO THREAD whenever the engine needs more samples:
    // fill `frames` (interleaved, pre-zeroed, frames.size() = frameCount * channels).
    // Do not call any p5cpp functions or allocate inside the callback.
    using AudioStreamCallback = std::function<void(std::span<float> frames, uint32_t channels)>;

    namespace detail
    {
        struct AudioStreamResource;
    }

    struct AudioStream;

    // Procedural audio: the callback fills sample buffers on demand (on the audio
    // thread - see AudioStreamCallback above). Returns an invalid AudioStream (see
    // isAudioStreamValid()) on failure.
    AudioStream createAudioStream(uint32_t sampleRate, uint32_t channels, AudioStreamCallback callback);

    // A procedural audio stream handle. Copies are cheap and alias the same
    // underlying miniaudio sound (shared_ptr-backed); everything is torn down
    // automatically once the last copy is destroyed. Default-constructed instances
    // are "invalid" - every free function below is a safe no-op on one (getters
    // return zero/default values) - see isAudioStreamValid().
    struct AudioStream
    {
        uint32_t sampleRate = 0;
        uint32_t channels = 0;

        // Internal handle driving automatic cleanup - not meant to be read or written
        // directly. Public (rather than private+friend) because detail::AudioStreamResource
        // is opaque outside audio_stream.cpp: exposing the pointer can't be used to
        // fabricate a working AudioStream, only to alias or null out this one.
        std::shared_ptr<detail::AudioStreamResource> resource;
    };

    bool isAudioStreamValid(const AudioStream& stream);

    void playAudioStream(const AudioStream& stream);
    void stopAudioStream(const AudioStream& stream);
    void pauseAudioStream(const AudioStream& stream);
    bool isPlaying(const AudioStream& stream);

    void setAudioStreamVolume(const AudioStream& stream, float volume);
    float getAudioStreamVolume(const AudioStream& stream);
    void setAudioStreamPan(const AudioStream& stream, float pan);
    float getAudioStreamPan(const AudioStream& stream);
    void setAudioStreamRate(const AudioStream& stream, float rate);
    float getAudioStreamRate(const AudioStream& stream);
} // namespace p5cpp
