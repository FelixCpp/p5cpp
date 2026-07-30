#pragma once

#include <p5cpp/audio/sound.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <vector>

struct ma_engine;

namespace p5cpp
{
    // Bridges miniaudio's audio-thread end notification to the main thread:
    // the audio thread only flips `pending`; AudioComponent::update() invokes
    // `callback` on the main thread. Owned by the sound impl (which guarantees
    // it outlives the underlying ma_sound), watched weakly by AudioComponent.
    struct SoundEndedState
    {
        std::atomic<bool> pending {false};
        std::function<void()> callback;
    };

    class AudioComponent
    {
    public:
        AudioComponent();
        ~AudioComponent();

        AudioComponent(const AudioComponent&) = delete;
        AudioComponent& operator=(const AudioComponent&) = delete;

        // Called once per frame (main thread): snapshots the audio-thread-written
        // waveform ring buffer into a stable buffer for getWaveform() to return,
        // dispatches pending onEnded callbacks, and reaps finished fire-and-forget
        // sounds started via playMulti().
        void update();

        void setMasterVolume(float volume);
        float getMasterVolume() const;

        float getAmplitude() const;
        std::span<const float> getWaveform() const;

        void watchEndedState(std::weak_ptr<SoundEndedState> state);
        void playMulti(const Sound& sound);

        ma_engine* getEngine();

    private:
        static constexpr size_t WAVEFORM_SIZE = 1024;

        static void onAudioProcess(void* userData, float* framesOut, uint64_t frameCount);
        void processAudioBlock(const float* framesOut, uint64_t frameCount);

        std::unique_ptr<ma_engine> m_engine;

        // Written on the audio thread inside onAudioProcess, read on the main thread
        // inside update()/getAmplitude(). memory_order_relaxed is sufficient: this is a
        // best-effort visualization/analysis feed, not a synchronization point for
        // playback-affecting state.
        std::array<std::atomic<float>, WAVEFORM_SIZE> m_waveformRing;
        std::atomic<size_t> m_waveformWriteIndex {0};
        std::atomic<float> m_amplitude {0.0f};

        std::vector<float> m_waveformSnapshot;

        // Main-thread only (registration and dispatch both happen there).
        std::vector<std::weak_ptr<SoundEndedState>> m_endedStates;
        std::vector<Sound> m_transientSounds;
    };

    // Resolves the AudioComponent registered on the current engine's AppContext -
    // shared by every audio loader (sound.cpp, audio_stream.cpp) and the
    // Component-forwarding functions in audio_api.cpp. Defined once in audio_api.cpp.
    AudioComponent& getAudioComponent();
} // namespace p5cpp
