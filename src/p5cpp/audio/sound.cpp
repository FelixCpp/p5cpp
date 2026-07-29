#include <p5cpp/audio/sound_impl.hpp>
#include <p5cpp/audio/audio_component.hpp>
#include <p5cpp/application/logging.hpp>

#include <miniaudio.h>

#include <utility>
#include <vector>

namespace p5cpp::detail
{
    // Real, actively-used state (not merely an id kept around for cleanup) - every
    // Sound method reaches into this through m_resource.
    struct SoundResource
    {
        // File-backed: the resource manager owns the underlying data source's lifetime
        // (ma_sound_uninit() frees it), so no decoder/backing-buffer ownership needed here.
        SoundResource(ma_engine* engine, AudioComponent* audio, std::unique_ptr<ma_sound> sound, bool streamed)
            : m_engine(engine),
              m_audio(audio),
              m_sound(std::move(sound)),
              m_streamed(streamed)
        {
        }

        // Memory-backed: ma_sound_init_from_data_source() only stores a pointer to the
        // decoder we hand it — it does not take ownership or copy the decoder's backing
        // bytes. Both must outlive the sound (it streams/decodes from them on demand for
        // as long as it plays), so this resource owns and outlives them. The bytes are
        // shared so clone() can decode from the same buffer.
        SoundResource(ma_engine* engine, AudioComponent* audio, std::unique_ptr<ma_sound> sound, std::unique_ptr<ma_decoder> decoder, std::shared_ptr<const std::vector<uint8_t>> encodedData)
            : m_engine(engine),
              m_audio(audio),
              m_sound(std::move(sound)),
              m_decoder(std::move(decoder)),
              m_encodedData(std::move(encodedData))
        {
        }

        // Sample-backed (createSound): the ma_audio_buffer reads directly from the shared
        // PCM vector, so both must outlive the sound; clones share the same PCM data.
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
            // Uninit the sound first: after ma_sound_uninit() returns, the audio thread
            // no longer touches the data source or fires the end callback, so the
            // decoder/audio buffer/ended state destroyed afterwards are safe to free.
            ma_sound_uninit(m_sound.get());
            if (m_decoder) {
                ma_decoder_uninit(m_decoder.get());
            }
            if (m_audioBuffer) {
                ma_audio_buffer_uninit(m_audioBuffer.get());
            }
        }

        void play() const
        {
            // Restarting a sound that ran to its natural end would otherwise start at
            // the end cursor and finish immediately.
            if (ma_sound_at_end(m_sound.get()) == MA_TRUE) {
                ma_sound_seek_to_pcm_frame(m_sound.get(), 0);
            }
            ma_sound_start(m_sound.get());
            m_status = SoundStatus::playing;
        }

        void stop() const
        {
            ma_sound_stop(m_sound.get());
            ma_sound_seek_to_pcm_frame(m_sound.get(), 0);
            m_status = SoundStatus::stopped;
        }

        void pause() const
        {
            ma_sound_stop(m_sound.get());
            m_status = SoundStatus::paused;
        }

        bool isPlaying() const
        {
            return ma_sound_is_playing(m_sound.get()) == MA_TRUE;
        }

        SoundStatus status() const
        {
            // miniaudio has no pause/stop distinction, so m_status tracks what the user
            // requested; a natural end is only observable through is_playing going false.
            if (m_status == SoundStatus::playing && ma_sound_is_playing(m_sound.get()) == MA_FALSE) {
                m_status = SoundStatus::stopped;
            }
            return m_status;
        }

        void setVolume(float volume) const
        {
            ma_sound_set_volume(m_sound.get(), volume);
        }

        float getVolume() const
        {
            return ma_sound_get_volume(m_sound.get());
        }

        void setPan(float pan) const
        {
            ma_sound_set_pan(m_sound.get(), pan);
        }

        float getPan() const
        {
            return ma_sound_get_pan(m_sound.get());
        }

        void setRate(float rate) const
        {
            ma_sound_set_pitch(m_sound.get(), rate);
        }

        float getRate() const
        {
            return ma_sound_get_pitch(m_sound.get());
        }

        void setLoop(bool loop) const
        {
            ma_sound_set_looping(m_sound.get(), loop ? MA_TRUE : MA_FALSE);
        }

        bool isLooping() const
        {
            return ma_sound_is_looping(m_sound.get()) == MA_TRUE;
        }

        void setLoopPoints(float startSeconds, float endSeconds) const
        {
            const uint32_t rate = queryFormat().first;
            ma_data_source* dataSource = ma_sound_get_data_source(m_sound.get());
            if (rate == 0 || dataSource == nullptr || endSeconds <= startSeconds) return;

            ma_data_source_set_loop_point_in_pcm_frames(
                dataSource,
                static_cast<ma_uint64>(startSeconds * static_cast<float>(rate)),
                static_cast<ma_uint64>(endSeconds * static_cast<float>(rate)));
        }

        void seek(float seconds) const
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

        void setFade(float fromVolume, float toVolume, float milliseconds) const
        {
            ma_sound_set_fade_in_milliseconds(m_sound.get(), fromVolume, toVolume, static_cast<ma_uint64>(milliseconds));
        }

        // (sampleRate, channels).
        std::pair<uint32_t, uint32_t> queryFormat() const
        {
            ma_uint32 rate = 0;
            ma_uint32 channelCount = 0;
            ma_sound_get_data_format(m_sound.get(), nullptr, &channelCount, &rate, nullptr, 0);
            return {rate, channelCount};
        }

        uint64_t frameCount() const
        {
            ma_uint64 length = 0;
            ma_sound_get_length_in_pcm_frames(m_sound.get(), &length);
            return length;
        }

        void onEnded(std::function<void()> callback) const
        {
            if (!m_endedState) {
                m_endedState = std::make_shared<SoundEndedState>();
                m_audio->watchEndedState(m_endedState);
                ma_sound_set_end_callback(m_sound.get(), &SoundResource::onSoundEnd, m_endedState.get());
            }
            m_endedState->callback = std::move(callback);
        }

        Sound clone() const;

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

        mutable SoundStatus m_status = SoundStatus::stopped;
        mutable std::shared_ptr<SoundEndedState> m_endedState;

    private:
        // Audio-thread side of onEnded(): only flips a flag, AudioComponent::update()
        // invokes the user callback on the main thread.
        static void onSoundEnd(void* userData, ma_sound*)
        {
            static_cast<SoundEndedState*>(userData)->pending.store(true, std::memory_order_relaxed);
        }
    };
} // namespace p5cpp::detail

namespace p5cpp
{
    namespace
    {
        std::shared_ptr<detail::SoundResource> makeMemorySound(ma_engine* engine, AudioComponent* audio, std::shared_ptr<const std::vector<uint8_t>> encodedData)
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

            return std::make_shared<detail::SoundResource>(engine, audio, std::move(sound), std::move(decoder), std::move(encodedData));
        }

        std::shared_ptr<detail::SoundResource> makePcmSound(ma_engine* engine, AudioComponent* audio, std::shared_ptr<const std::vector<float>> pcmData, uint32_t sampleRate, uint32_t channels)
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

            return std::make_shared<detail::SoundResource>(engine, audio, std::move(sound), std::move(audioBuffer), std::move(pcmData), sampleRate, channels);
        }
    } // namespace
} // namespace p5cpp

namespace p5cpp::detail
{
    Sound SoundResource::clone() const
    {
        if (m_pcmData) {
            std::shared_ptr<SoundResource> resource = makePcmSound(m_engine, m_audio, m_pcmData, m_pcmSampleRate, m_pcmChannels);
            if (!resource) return Sound();
            return Sound(m_pcmSampleRate, m_pcmChannels, std::move(resource));
        }

        if (m_encodedData) {
            std::shared_ptr<SoundResource> resource = makeMemorySound(m_engine, m_audio, m_encodedData);
            if (!resource) return Sound();
            const auto [sampleRate, channels] = resource->queryFormat();
            return Sound(sampleRate, channels, std::move(resource));
        }

        // ma_sound_init_copy() only supports non-streamed, resource-manager-loaded
        // sounds — the same restriction raylib's LoadSoundAlias has (Sound, not Music).
        if (m_streamed) {
            error("Sound::clone() is not supported for streamed music (loadMusic)");
            return Sound();
        }

        auto sound = std::make_unique<ma_sound>();
        if (ma_sound_init_copy(m_engine, m_sound.get(), MA_SOUND_FLAG_DECODE, nullptr, sound.get()) != MA_SUCCESS) {
            error("Failed to clone sound");
            return Sound();
        }

        auto resource = std::make_shared<SoundResource>(m_engine, m_audio, std::move(sound), false);
        const auto [sampleRate, channels] = resource->queryFormat();
        return Sound(sampleRate, channels, std::move(resource));
    }
} // namespace p5cpp::detail

namespace p5cpp
{
    Sound loadSoundImpl(AudioComponent& audio, const std::filesystem::path& soundFilePath, bool stream)
    {
        const ma_uint32 flags = stream ? MA_SOUND_FLAG_STREAM : MA_SOUND_FLAG_DECODE;

        auto sound = std::make_unique<ma_sound>();
        const ma_result result = ma_sound_init_from_file(audio.getEngine(), soundFilePath.string().c_str(), flags, nullptr, nullptr, sound.get());
        if (result != MA_SUCCESS) {
            error("Failed to load sound: " + soundFilePath.string());
            return Sound();
        }

        ma_uint32 sampleRate = 0;
        ma_uint32 channels = 0;
        ma_sound_get_data_format(sound.get(), nullptr, &channels, &sampleRate, nullptr, 0);

        auto resource = std::make_shared<detail::SoundResource>(audio.getEngine(), &audio, std::move(sound), stream);
        return Sound(sampleRate, channels, std::move(resource));
    }

    Sound loadSoundImpl(AudioComponent& audio, std::span<const uint8_t> soundData)
    {
        // ma_decoder_init_memory() does not copy the bytes — it reads from them on
        // demand as the sound plays, so we keep our own copy alive for the sound's
        // lifetime rather than relying on the caller's buffer to outlive playback.
        auto data = std::make_shared<const std::vector<uint8_t>>(soundData.begin(), soundData.end());
        std::shared_ptr<detail::SoundResource> resource = makeMemorySound(audio.getEngine(), &audio, std::move(data));
        if (!resource) {
            return Sound();
        }

        const auto [sampleRate, channels] = resource->queryFormat();
        return Sound(sampleRate, channels, std::move(resource));
    }

    Sound createSoundImpl(AudioComponent& audio, std::span<const float> samples, uint32_t sampleRate, uint32_t channels)
    {
        auto data = std::make_shared<const std::vector<float>>(samples.begin(), samples.end());
        std::shared_ptr<detail::SoundResource> resource = makePcmSound(audio.getEngine(), &audio, std::move(data), sampleRate, channels);
        if (!resource) {
            return Sound();
        }

        return Sound(sampleRate, channels, std::move(resource));
    }
} // namespace p5cpp

namespace p5cpp
{
    Sound::Sound() = default;

    Sound::Sound(uint32_t sampleRate, uint32_t channels, std::shared_ptr<detail::SoundResource> resource)
        : sampleRate(sampleRate),
          channels(channels),
          m_resource(std::move(resource))
    {
    }

    bool Sound::isValid() const { return m_resource != nullptr; }
    Sound::operator bool() const { return isValid(); }

    void Sound::play() const { if (m_resource) m_resource->play(); }
    void Sound::stop() const { if (m_resource) m_resource->stop(); }
    void Sound::pause() const { if (m_resource) m_resource->pause(); }
    bool Sound::isPlaying() const { return m_resource ? m_resource->isPlaying() : false; }
    SoundStatus Sound::status() const { return m_resource ? m_resource->status() : SoundStatus::stopped; }

    void Sound::setVolume(float volume) const { if (m_resource) m_resource->setVolume(volume); }
    float Sound::getVolume() const { return m_resource ? m_resource->getVolume() : 0.0f; }
    void Sound::setPan(float pan) const { if (m_resource) m_resource->setPan(pan); }
    float Sound::getPan() const { return m_resource ? m_resource->getPan() : 0.0f; }
    void Sound::setRate(float rate) const { if (m_resource) m_resource->setRate(rate); }
    float Sound::getRate() const { return m_resource ? m_resource->getRate() : 0.0f; }
    void Sound::setLoop(bool loop) const { if (m_resource) m_resource->setLoop(loop); }
    bool Sound::isLooping() const { return m_resource ? m_resource->isLooping() : false; }
    void Sound::setLoopPoints(float startSeconds, float endSeconds) const { if (m_resource) m_resource->setLoopPoints(startSeconds, endSeconds); }

    void Sound::seek(float seconds) const { if (m_resource) m_resource->seek(seconds); }
    float Sound::currentTime() const { return m_resource ? m_resource->currentTime() : 0.0f; }
    float Sound::duration() const { return m_resource ? m_resource->duration() : 0.0f; }

    void Sound::setFade(float fromVolume, float toVolume, float milliseconds) const { if (m_resource) m_resource->setFade(fromVolume, toVolume, milliseconds); }

    void Sound::fadeIn(float milliseconds) const
    {
        if (!m_resource) return;
        m_resource->setFade(0.0f, 1.0f, milliseconds);
        m_resource->play();
    }

    void Sound::fadeOut(float milliseconds) const
    {
        if (m_resource) m_resource->setFade(-1.0f, 0.0f, milliseconds);
    }

    uint64_t Sound::frameCount() const { return m_resource ? m_resource->frameCount() : 0; }

    void Sound::onEnded(std::function<void()> callback) const { if (m_resource) m_resource->onEnded(std::move(callback)); }

    Sound Sound::clone() const { return m_resource ? m_resource->clone() : Sound(); }
} // namespace p5cpp
