#pragma once

#include <p5cpp_audio/p5cpp_audio.hpp>

#include <miniaudio.h>

namespace p5::audio
{
    class AudioEngine
    {
    public:
        static std::unique_ptr<AudioEngine> create();

        AudioEngine(const AudioEngine&) = delete;
        AudioEngine& operator=(const AudioEngine&) = delete;
        ~AudioEngine();

        std::unique_ptr<SoundResource> loadSoundResource(const std::filesystem::path& path, const Bus& bus);

        void playSound(const Sound& sound);
        void stopSound(const Sound& sound);
        void pauseSound(const Sound& sound);
        void resumeSound(const Sound& sound);

        void setSoundVolume(const Sound& sound, float volume);
        void setSoundPan(const Sound& sound, float pan);
        void setSoundPitch(const Sound& sound, float pitch);

    private:
        AudioEngine() = default;

        // ma_engine_init() wires up internal self-pointers (e.g. the node graph's endpoint node
        // points back at &m_engine.nodeGraph), so the engine must be initialized in place at its
        // final address -- never built as a local/temporary and then copied or moved into m_engine,
        // which would leave those pointers dangling into freed memory. Same reasoning applies to
        // SoundResource::sound below.
        ma_engine m_engine {};
        bool m_initialized = false;
    };

    AudioEngine& getAudioEngine();
} // namespace p5::audio
