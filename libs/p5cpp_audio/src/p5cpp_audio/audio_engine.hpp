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

        Sound loadSoundFromFile(const std::filesystem::path& filepath);
        Sound loadSoundFromMemory(std::span<const uint8_t> data);
        Sound createSoundAlias(const Sound& sound);
        void playSound(const Sound& sound);
        void playSoundOverlapped(const Sound& sound);
        void pauseSound(const Sound& sound);
        void resumeSound(const Sound& sound);
        void stopSound(const Sound& sound);
        bool isSoundPlaying(const Sound& sound);
        PlaybackState getSoundPlaybackState(const Sound& sound);

        void setSoundVolume(const Sound& sound, float volume);
        void setSoundPitch(const Sound& sound, float pitch);
        void setSoundPan(const Sound& sound, float pan);

        void setSoundLoop(const Sound& sound, bool loop);
        bool isSoundLooping(const Sound& sound);

        void seekSound(const Sound& sound, float seconds);
        float getSoundTimePlayed(const Sound& sound);
        float getSoundTimeLength(const Sound& sound);

        void pruneFinishedOverlaps();

        MixedAudioProcessorHandle attachMixedAudioProcessor(MixedAudioProcessor processor);
        void detachMixedAudioProcessor(MixedAudioProcessorHandle handle);

        SoundProcessorHandle attachSoundProcessor(const Sound& sound, SoundProcessor processor);
        void detachSoundProcessor(const Sound& sound, SoundProcessorHandle handle);

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
