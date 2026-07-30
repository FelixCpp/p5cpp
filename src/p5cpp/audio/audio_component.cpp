#include <p5cpp/audio/audio_component.hpp>

#include <miniaudio.h>

#include <cmath>

namespace p5cpp
{
    AudioComponent::AudioComponent()
        : m_engine(std::make_unique<ma_engine>())
    {
        for (std::atomic<float>& sample : m_waveformRing) {
            sample.store(0.0f, std::memory_order_relaxed);
        }
        m_waveformSnapshot.assign(WAVEFORM_SIZE, 0.0f);

        ma_engine_config config = ma_engine_config_init();
        config.onProcess = &AudioComponent::onAudioProcess;
        config.pProcessUserData = this;

        ma_engine_init(&config, m_engine.get());
    }

    AudioComponent::~AudioComponent()
    {
        ma_engine_uninit(m_engine.get());
    }

    void AudioComponent::update()
    {
        const size_t writeIndex = m_waveformWriteIndex.load(std::memory_order_relaxed);
        for (size_t i = 0; i < WAVEFORM_SIZE; ++i) {
            const size_t idx = (writeIndex + i) % WAVEFORM_SIZE;
            m_waveformSnapshot[i] = m_waveformRing[idx].load(std::memory_order_relaxed);
        }

        std::erase_if(m_transientSounds, [](const Sound& sound) { return getSoundStatus(sound) != SoundStatus::playing; });
        std::erase_if(m_endedStates, [](const std::weak_ptr<SoundEndedState>& state) { return state.expired(); });

        // Iterate over a copy: a callback may load sounds or register new
        // onEnded handlers, which would mutate m_endedStates mid-loop.
        const std::vector<std::weak_ptr<SoundEndedState>> states = m_endedStates;
        for (const std::weak_ptr<SoundEndedState>& weakState : states) {
            const std::shared_ptr<SoundEndedState> state = weakState.lock();
            if (state && state->pending.exchange(false, std::memory_order_relaxed) && state->callback) {
                state->callback();
            }
        }
    }

    void AudioComponent::watchEndedState(std::weak_ptr<SoundEndedState> state)
    {
        m_endedStates.push_back(std::move(state));
    }

    void AudioComponent::playMulti(const Sound& sound)
    {
        Sound transient = cloneSound(sound);
        if (!isSoundValid(transient)) return;

        playSound(transient);
        m_transientSounds.push_back(std::move(transient));
    }

    void AudioComponent::setMasterVolume(float volume)
    {
        ma_engine_set_volume(m_engine.get(), volume);
    }

    float AudioComponent::getMasterVolume() const
    {
        return ma_engine_get_volume(m_engine.get());
    }

    float AudioComponent::getAmplitude() const
    {
        return m_amplitude.load(std::memory_order_relaxed);
    }

    std::span<const float> AudioComponent::getWaveform() const
    {
        return std::span<const float>(m_waveformSnapshot);
    }

    ma_engine* AudioComponent::getEngine()
    {
        return m_engine.get();
    }

    void AudioComponent::onAudioProcess(void* userData, float* framesOut, uint64_t frameCount)
    {
        static_cast<AudioComponent*>(userData)->processAudioBlock(framesOut, frameCount);
    }

    void AudioComponent::processAudioBlock(const float* framesOut, uint64_t frameCount)
    {
        const ma_uint32 channels = ma_engine_get_channels(m_engine.get());
        if (channels == 0 || frameCount == 0) return;

        float sumSquares = 0.0f;

        for (uint64_t i = 0; i < frameCount; ++i) {
            float sample = 0.0f;
            for (ma_uint32 c = 0; c < channels; ++c) {
                sample += framesOut[i * channels + c];
            }
            sample /= static_cast<float>(channels);
            sumSquares += sample * sample;

            const size_t idx = m_waveformWriteIndex.fetch_add(1, std::memory_order_relaxed) % WAVEFORM_SIZE;
            m_waveformRing[idx].store(sample, std::memory_order_relaxed);
        }

        m_amplitude.store(std::sqrt(sumSquares / static_cast<float>(frameCount)), std::memory_order_relaxed);
    }
} // namespace p5cpp
