#include <p5cpp_audio/audio_engine.hpp>
#include <p5cpp_audio/sound_resource.hpp>

namespace p5::audio
{
    std::unique_ptr<AudioEngine> AudioEngine::create()
    {
        std::unique_ptr<AudioEngine> engine {new AudioEngine()};

        ma_engine_config config = ma_engine_config_init();
        config.onProcess = &AudioEngine::onEngineProcess;
        config.pProcessUserData = engine.get();

        if (ma_engine_init(&config, &engine->m_engine) != MA_SUCCESS) {
            error("Failed to initialize audio engine");
            return nullptr;
        }

        return engine;
    }

    AudioEngine::~AudioEngine()
    {
        ma_engine_uninit(&m_engine);
    }

    void AudioEngine::setMasterVolume(const float volume)
    {
        ma_engine_set_volume(&m_engine, volume);
    }

    float AudioEngine::getMasterVolume()
    {
        return ma_engine_get_volume(&m_engine);
    }

    std::optional<Sound> AudioEngine::loadSound(const std::filesystem::path& filepath)
    {
        std::unique_ptr<SoundResource> resource = SoundResource::loadFromFile(m_engine, filepath);
        if (resource == nullptr) {
            return std::nullopt;
        }

        return Sound {.resource = std::move(resource)};
    }

    std::optional<Sound> AudioEngine::loadSound(const std::span<const uint8_t> data)
    {
        std::unique_ptr<SoundResource> resource = SoundResource::loadFromMemory(m_engine, data);
        if (resource == nullptr) {
            return std::nullopt;
        }

        return Sound {.resource = std::move(resource)};
    }

    Sound AudioEngine::createAlias(const Sound& sound)
    {
        return Sound {
            .resource = SoundResource::createAlias(m_engine, *sound.resource),
        };
    }

    void AudioEngine::playOverlapped(const Sound& sound)
    {
        Sound alias = createAlias(sound);
        alias.resource->play();
        m_activeOverlaps.push_back(std::move(alias));
    }

    void AudioEngine::pruneFinishedOverlaps()
    {
        std::erase_if(m_activeOverlaps, [](const Sound& sound) {
            return not sound.resource->isPlaying();
        });
    }

    void AudioEngine::play(const Sound& sound) { sound.resource->play(); }
    void AudioEngine::pause(const Sound& sound) { sound.resource->pause(); }
    void AudioEngine::resume(const Sound& sound) { sound.resource->resume(); }
    void AudioEngine::stop(const Sound& sound) { sound.resource->stop(); }
    bool AudioEngine::isPlaying(const Sound& sound) { return sound.resource->isPlaying(); }

    PlaybackState AudioEngine::getPlaybackState(const Sound& sound)
    {
        if (sound.resource->isPlaying()) {
            return PlaybackState::Playing;
        }

        if (sound.resource->isAtEnd() or sound.resource->getCursorInFrames() == 0) {
            return PlaybackState::Stopped;
        }

        return PlaybackState::Paused;
    }

    MixedAudioProcessorHandle AudioEngine::attachMixedAudioProcessor(MixedAudioProcessor processor)
    {
        std::lock_guard lock {m_processorsMutex};
        const uint64_t id = m_nextProcessorId++;
        m_processors.emplace_back(id, std::move(processor));

        return MixedAudioProcessorHandle {.id = id};
    }

    void AudioEngine::detachMixedAudioProcessor(const MixedAudioProcessorHandle handle)
    {
        std::lock_guard lock {m_processorsMutex};
        std::erase_if(m_processors, [handle](const auto& entry) {
            return entry.first == handle.id;
        });
    }

    void AudioEngine::onEngineProcess(void* const userData, float* const framesOut, const ma_uint64 frameCount)
    {
        auto& self = *static_cast<AudioEngine*>(userData);
        const auto channels = ma_engine_get_channels(&self.m_engine);
        const std::span<float> frames {framesOut, static_cast<size_t>(frameCount) * channels};

        std::lock_guard lock {self.m_processorsMutex};
        for (const auto& [id, processor] : self.m_processors) {
            processor(frames, channels);
        }
    }

    void AudioEngine::setVolume(const Sound& sound, const float volume) { sound.resource->setVolume(volume); }
    void AudioEngine::setPitch(const Sound& sound, const float pitch) { sound.resource->setPitch(pitch); }
    void AudioEngine::setPan(const Sound& sound, const float pan) { sound.resource->setPan(pan); }

    void AudioEngine::setLoop(const Sound& sound, const bool loop) { sound.resource->setLooping(loop); }
    bool AudioEngine::isLooping(const Sound& sound) { return sound.resource->isLooping(); }

    void AudioEngine::seek(const Sound& sound, const float seconds) { sound.resource->seek(seconds); }
    float AudioEngine::getTimePlayed(const Sound& sound) { return sound.resource->getTimePlayed(); }
    float AudioEngine::getTimeLength(const Sound& sound) { return sound.resource->getTimeLength(); }

    SoundProcessorHandle AudioEngine::attachProcessor(const Sound& sound, SoundProcessor processor)
    {
        return sound.resource->attachProcessor(std::move(processor));
    }

    void AudioEngine::detachProcessor(const Sound& sound, const SoundProcessorHandle handle)
    {
        sound.resource->detachProcessor(handle);
    }

    AudioEngine::AudioEngine() = default;
} // namespace p5::audio
