#pragma once

#include <p5cpp/p5cpp.hpp>

namespace p5::audio
{
    enum class PlaybackState
    {
        Stopped, // never played, or explicitly stopped via stop() -- position is at the start
        Playing, // currently playing, not stopped or paused
        Paused,  // paused via pause() -- position is preserved, ready to continue via resume()
    };

    using SoundProcessor = std::function<void(std::span<float> frames, uint32_t channels)>;
    struct SoundProcessorHandle
    {
        uint64_t id = 0;
    };

    struct SoundResource;
    struct Sound
    {
        std::shared_ptr<SoundResource> resource;

        bool isValid() const;

        void play();
        void playOverlapped();
        void pause();
        void resume();
        void stop();
        bool isPlaying() const;
        PlaybackState getPlaybackState() const;

        void setVolume(float volume);
        void setPitch(float pitch);
        void setPan(float pan);

        void setLoop(bool loop);
        bool isLooping() const;

        void seek(float seconds);
        float getTimePlayed() const;
        float getTimeLength() const;

        Sound createAlias() const;

        SoundProcessorHandle attachProcessor(SoundProcessor processor);
        void detachProcessor(SoundProcessorHandle handle);
    };

    std::optional<Sound> loadSound(const std::filesystem::path& filepath);
    std::optional<Sound> loadSound(std::span<const uint8_t> data);

    void setMasterVolume(float volume);
    float getMasterVolume();

    using MixedAudioProcessor = std::function<void(std::span<float> frames, uint32_t channels)>;
    struct MixedAudioProcessorHandle
    {
        uint64_t id = 0;
    };

    MixedAudioProcessorHandle attachMixedAudioProcessor(MixedAudioProcessor processor);
    void detachMixedAudioProcessor(MixedAudioProcessorHandle handle);
} // namespace p5::audio

namespace p5::audio
{
    std::unique_ptr<Plugin> createAudioPlugin();
} // namespace p5::audio
