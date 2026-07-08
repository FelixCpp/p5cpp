#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

struct ma_engine;

namespace p5cpp
{
    class AudioComponent
    {
    public:
        AudioComponent();
        ~AudioComponent();

        AudioComponent(const AudioComponent&) = delete;
        AudioComponent& operator=(const AudioComponent&) = delete;

        // Called once per frame (main thread): snapshots the audio-thread-written
        // waveform ring buffer into a stable buffer for getWaveform() to return.
        void update();

        void setMasterVolume(float volume);
        float getMasterVolume() const;

        float getAmplitude() const;
        std::span<const float> getWaveform() const;

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
    };
} // namespace p5cpp
