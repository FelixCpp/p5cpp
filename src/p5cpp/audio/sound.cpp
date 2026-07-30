#include <p5cpp/audio/sound.hpp>
#include <p5cpp/audio/audio_component.hpp>
#include <p5cpp/application/logging.hpp>

#include <miniaudio.h>

#include <vector>

namespace p5cpp::detail
{
    struct SoundResource
    {
        SoundResource(ma_engine* engine, AudioComponent* audio, std::unique_ptr<ma_sound> sound, bool streamed)
            : m_engine(engine),
              m_audio(audio),
              m_sound(std::move(sound)),
              m_streamed(streamed)
        {
        }

        SoundResource(ma_engine* engine, AudioComponent* audio, std::unique_ptr<ma_sound> sound, std::unique_ptr<ma_decoder> decoder, std::shared_ptr<const std::vector<uint8_t>> encodedData)
            : m_engine(engine),
              m_audio(audio),
              m_sound(std::move(sound)),
              m_decoder(std::move(decoder)),
              m_encodedData(std::move(encodedData))
        {
        }

        SoundResource(ma_engine* engine, AudioComponent* audio, std::unique_ptr<ma_sound> sound, std::unique_ptr<ma_audio_buffer> audioBuffer, std::shared_ptr<const std::vector<float>> pcmData, uint32_t pcmSampleRate, uint32_t pcmChannels)
            : m_engine(engine),
              m_audio(audio),
              m_sound(std::move(sound)),
              m_audioBuffer(std::move(audioBuffer)),
              m_pcmData(std::move(pcmData)),
              m_pcmSampleRate(pcmSampleRate),
              m_pcmChannels(pcmChannels)
        {
        }

        ~SoundResource()
        {
            ma_sound_uninit(m_sound.get());

            if (m_decoder) {
                ma_decoder_uninit(m_decoder.get());
            }
            if (m_audioBuffer) {
                ma_audio_buffer_uninit(m_audioBuffer.get());
            }
        }

        void play()
        {
            if (ma_sound_at_end(m_sound.get()) == MA_TRUE) {
                ma_sound_seek_to_pcm_frame(m_sound.get(), 0);
            }

            ma_sound_start(m_sound.get());
            m_status = SoundStatus::playing;
        }

        void stop()
        {
            ma_sound_stop(m_sound.get());
            ma_sound_seek_to_pcm_frame(m_sound.get(), 0);
            m_status = SoundStatus::stopped;
        }

        void pause()
        {
            ma_sound_stop(m_sound.get());
            m_status = SoundStatus::paused;
        }

        bool isPlaying() const
        {
            return ma_sound_is_playing(m_sound.get()) == MA_TRUE;
        }

        SoundStatus status()
        {
            if (m_status == SoundStatus::playing && ma_sound_is_playing(m_sound.get()) == MA_FALSE) {
                m_status = SoundStatus::stopped;
            }
            return m_status;
        }

        void setVolume(float volume)
        {
            ma_sound_set_volume(m_sound.get(), volume);
        }

        float getVolume() const
        {
            return ma_sound_get_volume(m_sound.get());
        }

        void setPan(float pan)
        {
            ma_sound_set_pan(m_sound.get(), pan);
        }

        float getPan() const
        {
            return ma_sound_get_pan(m_sound.get());
        }

        void setRate(float rate)
        {
            ma_sound_set_pitch(m_sound.get(), rate);
        }

        float getRate() const
        {
            return ma_sound_get_pitch(m_sound.get());
        }

        void setLoop(bool loop)
        {
            ma_sound_set_looping(m_sound.get(), loop ? MA_TRUE : MA_FALSE);
        }

        bool isLooping() const
        {
            return ma_sound_is_looping(m_sound.get()) == MA_TRUE;
        }

        void setLoopPoints(float startSeconds, float endSeconds)
        {
            const uint32_t rate = sampleRate();
            ma_data_source* dataSource = ma_sound_get_data_source(m_sound.get());
            if (rate == 0 || dataSource == nullptr || endSeconds <= startSeconds) return;

            ma_data_source_set_loop_point_in_pcm_frames(
                dataSource,
                static_cast<ma_uint64>(startSeconds * static_cast<float>(rate)),
                static_cast<ma_uint64>(endSeconds * static_cast<float>(rate))
            );
        }

        void seek(float seconds)
        {
            ma_sound_seek_to_second(m_sound.get(), seconds);
        }

        float currentTime() const
        {
            float cursor = 0.0f;
            ma_sound_get_cursor_in_seconds(m_sound.get(), &cursor);
            return cursor;
        }

        float duration() const
        {
            float length = 0.0f;
            ma_sound_get_length_in_seconds(m_sound.get(), &length);
            return length;
        }

        void setFade(float fromVolume, float toVolume, float milliseconds)
        {
            ma_sound_set_fade_in_milliseconds(m_sound.get(), fromVolume, toVolume, static_cast<ma_uint64>(milliseconds));
        }

        uint32_t sampleRate() const
        {
            ma_uint32 rate = 0;
            ma_sound_get_data_format(m_sound.get(), nullptr, nullptr, &rate, nullptr, 0);
            return rate;
        }

        uint32_t channels() const
        {
            ma_uint32 channelCount = 0;
            ma_sound_get_data_format(m_sound.get(), nullptr, &channelCount, nullptr, nullptr, 0);
            return channelCount;
        }

        uint64_t frameCount() const
        {
            ma_uint64 length = 0;
            ma_sound_get_length_in_pcm_frames(m_sound.get(), &length);
            return length;
        }

        void onEnded(std::function<void()> callback)
        {
            if (!m_endedState) {
                m_endedState = std::make_shared<SoundEndedState>();
                m_audio->watchEndedState(m_endedState);
                ma_sound_set_end_callback(m_sound.get(), &SoundResource::onSoundEnd, m_endedState.get());
            }
            m_endedState->callback = std::move(callback);
        }

        std::shared_ptr<SoundResource> clone() const;

        ma_engine* m_engine = nullptr;
        AudioComponent* m_audio = nullptr;
        std::unique_ptr<ma_sound> m_sound;
        std::unique_ptr<ma_decoder> m_decoder;
        std::shared_ptr<const std::vector<uint8_t>> m_encodedData;
        std::unique_ptr<ma_audio_buffer> m_audioBuffer;
        std::shared_ptr<const std::vector<float>> m_pcmData;
        uint32_t m_pcmSampleRate = 0;
        uint32_t m_pcmChannels = 0;
        bool m_streamed = false;

        SoundStatus m_status = SoundStatus::stopped;
        std::shared_ptr<SoundEndedState> m_endedState;

    private:
        static void onSoundEnd(void* userData, ma_sound*)
        {
            static_cast<SoundEndedState*>(userData)->pending.store(true, std::memory_order_relaxed);
        }
    };
} // namespace p5cpp::detail

namespace p5cpp::detail
{
    namespace
    {
        std::shared_ptr<SoundResource> makeMemorySound(ma_engine* engine, AudioComponent* audio, std::shared_ptr<const std::vector<uint8_t>> encodedData)
        {
            auto decoder = std::make_unique<ma_decoder>();
            const ma_result decoderResult = ma_decoder_init_memory(encodedData->data(), encodedData->size(), nullptr, decoder.get());
            if (decoderResult != MA_SUCCESS) {
                error("Failed to decode in-memory sound data");
                return nullptr;
            }

            auto sound = std::make_unique<ma_sound>();
            const ma_result result = ma_sound_init_from_data_source(engine, decoder.get(), 0, nullptr, sound.get());
            if (result != MA_SUCCESS) {
                ma_decoder_uninit(decoder.get());
                error("Failed to load sound from memory");
                return nullptr;
            }

            return std::make_shared<SoundResource>(engine, audio, std::move(sound), std::move(decoder), std::move(encodedData));
        }

        std::shared_ptr<SoundResource> makePcmSound(ma_engine* engine, AudioComponent* audio, std::shared_ptr<const std::vector<float>> pcmData, uint32_t sampleRate, uint32_t channels)
        {
            if (channels == 0 || sampleRate == 0 || pcmData->empty()) {
                error("createSound: samples, sampleRate and channels must be non-zero");
                return nullptr;
            }

            auto audioBuffer = std::make_unique<ma_audio_buffer>();
            ma_audio_buffer_config config = ma_audio_buffer_config_init(ma_format_f32, channels, pcmData->size() / channels, pcmData->data(), nullptr);
            config.sampleRate = sampleRate;
            if (ma_audio_buffer_init(&config, audioBuffer.get()) != MA_SUCCESS) {
                error("createSound: failed to initialize audio buffer");
                return nullptr;
            }

            auto sound = std::make_unique<ma_sound>();
            if (ma_sound_init_from_data_source(engine, audioBuffer.get(), 0, nullptr, sound.get()) != MA_SUCCESS) {
                ma_audio_buffer_uninit(audioBuffer.get());
                error("createSound: failed to create sound from samples");
                return nullptr;
            }

            return std::make_shared<SoundResource>(engine, audio, std::move(sound), std::move(audioBuffer), std::move(pcmData), sampleRate, channels);
        }
    } // namespace

    std::shared_ptr<SoundResource> SoundResource::clone() const
    {
        if (m_pcmData) {
            return makePcmSound(m_engine, m_audio, m_pcmData, m_pcmSampleRate, m_pcmChannels);
        }

        if (m_encodedData) {
            return makeMemorySound(m_engine, m_audio, m_encodedData);
        }

        // ma_sound_init_copy() only supports non-streamed, resource-manager-loaded
        // sounds — the same restriction raylib's LoadSoundAlias has (Sound, not Music).
        if (m_streamed) {
            error("Sound::clone() is not supported for streamed music (loadMusic)");
            return nullptr;
        }

        auto sound = std::make_unique<ma_sound>();
        if (ma_sound_init_copy(m_engine, m_sound.get(), MA_SOUND_FLAG_DECODE, nullptr, sound.get()) != MA_SUCCESS) {
            error("Failed to clone sound");
            return nullptr;
        }

        return std::make_shared<SoundResource>(m_engine, m_audio, std::move(sound), false);
    }
} // namespace p5cpp::detail

namespace p5cpp
{
    Sound loadSound(const std::filesystem::path& soundFilePath)
    {
        AudioComponent& audio = getAudioComponent();

        auto sound = std::make_unique<ma_sound>();
        const ma_result result = ma_sound_init_from_file(audio.getEngine(), soundFilePath.string().c_str(), MA_SOUND_FLAG_DECODE, nullptr, nullptr, sound.get());
        if (result != MA_SUCCESS) {
            error("Failed to load sound: " + soundFilePath.string());
            return Sound();
        }

        auto resource = std::make_shared<detail::SoundResource>(audio.getEngine(), &audio, std::move(sound), false);
        Sound result_;
        result_.sampleRate = resource->sampleRate();
        result_.channels = resource->channels();
        result_.resource = std::move(resource);
        return result_;
    }

    Sound loadSound(std::span<const uint8_t> soundData)
    {
        AudioComponent& audio = getAudioComponent();

        auto data = std::make_shared<const std::vector<uint8_t>>(soundData.begin(), soundData.end());
        std::shared_ptr<detail::SoundResource> resource = detail::makeMemorySound(audio.getEngine(), &audio, std::move(data));
        if (!resource) {
            return Sound();
        }

        Sound result;
        result.sampleRate = resource->sampleRate();
        result.channels = resource->channels();
        result.resource = std::move(resource);
        return result;
    }

    Sound loadMusic(const std::filesystem::path& musicFilePath)
    {
        AudioComponent& audio = getAudioComponent();

        auto sound = std::make_unique<ma_sound>();
        const ma_result result = ma_sound_init_from_file(audio.getEngine(), musicFilePath.string().c_str(), MA_SOUND_FLAG_STREAM, nullptr, nullptr, sound.get());
        if (result != MA_SUCCESS) {
            error("Failed to load sound: " + musicFilePath.string());
            return Sound();
        }

        auto resource = std::make_shared<detail::SoundResource>(audio.getEngine(), &audio, std::move(sound), true);
        Sound result_;
        result_.sampleRate = resource->sampleRate();
        result_.channels = resource->channels();
        result_.resource = std::move(resource);
        return result_;
    }

    Sound createSound(std::span<const float> samples, uint32_t sampleRate, uint32_t channels)
    {
        AudioComponent& audio = getAudioComponent();

        auto data = std::make_shared<const std::vector<float>>(samples.begin(), samples.end());
        std::shared_ptr<detail::SoundResource> resource = detail::makePcmSound(audio.getEngine(), &audio, std::move(data), sampleRate, channels);
        if (!resource) {
            return Sound();
        }

        Sound result;
        result.sampleRate = resource->sampleRate();
        result.channels = resource->channels();
        result.resource = std::move(resource);
        return result;
    }
} // namespace p5cpp

namespace p5cpp
{
    bool isSoundValid(const Sound& sound) { return sound.resource != nullptr; }

    void playSound(const Sound& sound)
    {
        if (sound.resource) sound.resource->play();
    }
    void stopSound(const Sound& sound)
    {
        if (sound.resource) sound.resource->stop();
    }
    void pauseSound(const Sound& sound)
    {
        if (sound.resource) sound.resource->pause();
    }
    bool isPlaying(const Sound& sound) { return sound.resource ? sound.resource->isPlaying() : false; }
    SoundStatus getSoundStatus(const Sound& sound) { return sound.resource ? sound.resource->status() : SoundStatus::stopped; }

    void setSoundVolume(const Sound& sound, float volume)
    {
        if (sound.resource) sound.resource->setVolume(volume);
    }
    float getSoundVolume(const Sound& sound) { return sound.resource ? sound.resource->getVolume() : 0.0f; }
    void setSoundPan(const Sound& sound, float pan)
    {
        if (sound.resource) sound.resource->setPan(pan);
    }
    float getSoundPan(const Sound& sound) { return sound.resource ? sound.resource->getPan() : 0.0f; }
    void setSoundRate(const Sound& sound, float rate)
    {
        if (sound.resource) sound.resource->setRate(rate);
    }
    float getSoundRate(const Sound& sound) { return sound.resource ? sound.resource->getRate() : 0.0f; }
    void setSoundLoop(const Sound& sound, bool loop)
    {
        if (sound.resource) sound.resource->setLoop(loop);
    }
    bool isSoundLooping(const Sound& sound) { return sound.resource ? sound.resource->isLooping() : false; }
    void setSoundLoopPoints(const Sound& sound, float startSeconds, float endSeconds)
    {
        if (sound.resource) sound.resource->setLoopPoints(startSeconds, endSeconds);
    }

    void seekSound(const Sound& sound, float seconds)
    {
        if (sound.resource) sound.resource->seek(seconds);
    }
    float getSoundCurrentTime(const Sound& sound) { return sound.resource ? sound.resource->currentTime() : 0.0f; }
    float getSoundDuration(const Sound& sound) { return sound.resource ? sound.resource->duration() : 0.0f; }

    void setSoundFade(const Sound& sound, float fromVolume, float toVolume, float milliseconds)
    {
        if (sound.resource) sound.resource->setFade(fromVolume, toVolume, milliseconds);
    }

    void fadeInSound(const Sound& sound, float milliseconds)
    {
        if (!sound.resource) return;
        sound.resource->setFade(0.0f, 1.0f, milliseconds);
        sound.resource->play();
    }

    void fadeOutSound(const Sound& sound, float milliseconds)
    {
        if (sound.resource) sound.resource->setFade(-1.0f, 0.0f, milliseconds);
    }

    uint64_t getSoundFrameCount(const Sound& sound) { return sound.resource ? sound.resource->frameCount() : 0; }

    void onSoundEnded(const Sound& sound, std::function<void()> callback)
    {
        if (sound.resource) sound.resource->onEnded(std::move(callback));
    }

    Sound cloneSound(const Sound& sound)
    {
        if (!sound.resource) {
            return Sound();
        }

        std::shared_ptr<detail::SoundResource> cloned = sound.resource->clone();
        if (!cloned) {
            return Sound();
        }

        Sound result;
        result.sampleRate = cloned->sampleRate();
        result.channels = cloned->channels();
        result.resource = std::move(cloned);
        return result;
    }
} // namespace p5cpp
