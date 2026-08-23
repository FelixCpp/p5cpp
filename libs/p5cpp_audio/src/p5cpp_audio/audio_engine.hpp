#pragma once

#include <p5cpp_audio/p5cpp_audio.hpp>

#include <miniaudio.h>

#include <memory>
#include <filesystem>
#include <vector>
#include <span>
#include <cstdint>

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

        void pruneFinishedOverlaps();

    private:
        explicit AudioEngine();

        ma_engine m_engine;
        std::vector<Sound> m_activeOverlaps;
    };

    AudioEngine& getAudioEngine();
} // namespace p5::audio
