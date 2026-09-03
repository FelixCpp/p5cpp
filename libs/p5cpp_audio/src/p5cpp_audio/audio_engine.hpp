#pragma once

#include <p5cpp_audio/p5cpp_audio.hpp>

#include <miniaudio.h>

#include <memory>
#include <filesystem>
#include <vector>
#include <span>
#include <cstdint>
#include <mutex>
#include <utility>

namespace p5::audio
{
    class AudioEngine
    {
    public:
        static std::unique_ptr<AudioEngine> create();
        ~AudioEngine();

        void setMasterVolume(float volume);
        float getMasterVolume();

        std::optional<Sound> loadSound(const std::filesystem::path& filepath);
        std::optional<Sound> loadSound(std::span<const uint8_t> data);
        Sound createAlias(const Sound& sound);
        void play(const Sound& sound);
        void playOverlapped(const Sound& sound);
        void pause(const Sound& sound);
        void resume(const Sound& sound);
        void stop(const Sound& sound);
        bool isPlaying(const Sound& sound);
        PlaybackState getPlaybackState(const Sound& sound);

        void setVolume(const Sound& sound, float volume);
        void setPitch(const Sound& sound, float pitch);
        void setPan(const Sound& sound, float pan);

        void setLoop(const Sound& sound, bool loop);
        bool isLooping(const Sound& sound);

        void seek(const Sound& sound, float seconds);
        float getTimePlayed(const Sound& sound);
        float getTimeLength(const Sound& sound);

        void pruneFinishedOverlaps();

        MixedAudioProcessorHandle attachMixedAudioProcessor(MixedAudioProcessor processor);
        void detachMixedAudioProcessor(MixedAudioProcessorHandle handle);

        SoundProcessorHandle attachProcessor(const Sound& sound, SoundProcessor processor);
        void detachProcessor(const Sound& sound, SoundProcessorHandle handle);

    private:
        explicit AudioEngine();

        static void onEngineProcess(void* userData, float* framesOut, ma_uint64 frameCount);

        ma_engine m_engine;
        std::vector<Sound> m_activeOverlaps;

        std::mutex m_processorsMutex; // guards m_processors; locked briefly on the audio thread too
        std::vector<std::pair<uint64_t, MixedAudioProcessor>> m_processors;
        uint64_t m_nextProcessorId = 1;
    };

    AudioEngine& getAudioEngine();
} // namespace p5::audio
