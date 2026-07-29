#pragma once

#include <p5cpp/audio/sound.hpp>
#include <p5cpp/audio/audio_stream.hpp>

#include <cstdint>
#include <filesystem>
#include <span>

namespace p5cpp
{
    class AudioComponent;

    // `stream` selects MA_SOUND_FLAG_STREAM (decode on the fly while playing)
    // instead of MA_SOUND_FLAG_DECODE (fully decode into memory up front).
    Sound loadSoundImpl(AudioComponent& audio, const std::filesystem::path& soundFilePath, bool stream);
    Sound loadSoundImpl(AudioComponent& audio, std::span<const uint8_t> soundData);
    Sound createSoundImpl(AudioComponent& audio, std::span<const float> samples, uint32_t sampleRate, uint32_t channels);

    AudioStream createAudioStreamImpl(AudioComponent& audio, uint32_t sampleRate, uint32_t channels, AudioStreamCallback callback);

    AudioSamples decodeAudioSamples(const std::filesystem::path& soundFilePath);
    AudioSamples decodeAudioSamples(std::span<const uint8_t> soundData);
} // namespace p5cpp
