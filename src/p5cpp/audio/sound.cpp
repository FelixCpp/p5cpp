#include <p5cpp/audio/sound_impl.hpp>
#include <p5cpp/audio/audio_component.hpp>
#include <p5cpp/application/logging.hpp>

#include <miniaudio.h>

#include <vector>

namespace p5cpp
{
    class MiniaudioSoundImpl : public SoundImpl
    {
    public:
        // File-backed: the resource manager owns the underlying data source's lifetime
        // (ma_sound_uninit() frees it), so no decoder/backing-buffer ownership needed here.
        MiniaudioSoundImpl(ma_engine* engine, AudioComponent* audio, std::unique_ptr<ma_sound> sound, bool streamed)
            : m_engine(engine),
              m_audio(audio),
              m_sound(std::move(sound)),
              m_streamed(streamed)
        {
        }

        // Memory-backed: ma_sound_init_from_data_source() only stores a pointer to the
        // decoder we hand it — it does not take ownership or copy the decoder's backing
        // bytes. Both must outlive the sound (it streams/decodes from them on demand for
        // as long as it plays), so this impl owns and outlives them. The bytes are shared
        // so clone() can decode from the same buffer.
        MiniaudioSoundImpl(ma_engine* engine, AudioComponent* audio, std::unique_ptr<ma_sound> sound, std::unique_ptr<ma_decoder> decoder, std::shared_ptr<const std::vector<uint8_t>> encodedData)
            : m_engine(engine),
              m_audio(audio),
              m_sound(std::move(sound)),
              m_decoder(std::move(decoder)),
              m_encodedData(std::move(encodedData))
        {
        }

        // Sample-backed (createSound): the ma_audio_buffer reads directly from the shared
        // PCM vector, so both must outlive the sound; clones share the same PCM data.
        MiniaudioSoundImpl(ma_engine* engine, AudioComponent* audio, std::unique_ptr<ma_sound> sound, std::unique_ptr<ma_audio_buffer> audioBuffer, std::shared_ptr<const std::vector<float>> pcmData, uint32_t pcmSampleRate, uint32_t pcmChannels)
            : m_engine(engine),
              m_audio(audio),
              m_sound(std::move(sound)),
              m_audioBuffer(std::move(audioBuffer)),
              m_pcmData(std::move(pcmData)),
              m_pcmSampleRate(pcmSampleRate),
              m_pcmChannels(pcmChannels)
        {
        }

        ~MiniaudioSoundImpl() override
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

        void play() const override
        {
            // Restarting a sound that ran to its natural end would otherwise start at
            // the end cursor and finish immediately.
            if (ma_sound_at_end(m_sound.get()) == MA_TRUE) {
                ma_sound_seek_to_pcm_frame(m_sound.get(), 0);
            }
            ma_sound_start(m_sound.get());
            m_status = SoundStatus::playing;
        }

        void stop() const override
        {
            ma_sound_stop(m_sound.get());
            ma_sound_seek_to_pcm_frame(m_sound.get(), 0);
            m_status = SoundStatus::stopped;
        }

        void pause() const override
        {
            ma_sound_stop(m_sound.get());
            m_status = SoundStatus::paused;
        }

        bool isPlaying() const override
        {
            return ma_sound_is_playing(m_sound.get()) == MA_TRUE;
        }

        SoundStatus status() const override
        {
            // miniaudio has no pause/stop distinction, so m_status tracks what the user
            // requested; a natural end is only observable through is_playing going false.
            if (m_status == SoundStatus::playing && ma_sound_is_playing(m_sound.get()) == MA_FALSE) {
                m_status = SoundStatus::stopped;
            }
            return m_status;
        }

        void setVolume(float volume) const override
        {
            ma_sound_set_volume(m_sound.get(), volume);
        }

        float getVolume() const override
        {
            return ma_sound_get_volume(m_sound.get());
        }

        void setPan(float pan) const override
        {
            ma_sound_set_pan(m_sound.get(), pan);
        }

        float getPan() const override
        {
            return ma_sound_get_pan(m_sound.get());
        }

        void setRate(float rate) const override
        {
            ma_sound_set_pitch(m_sound.get(), rate);
        }

        float getRate() const override
        {
            return ma_sound_get_pitch(m_sound.get());
        }

        void setLoop(bool loop) const override
        {
            ma_sound_set_looping(m_sound.get(), loop ? MA_TRUE : MA_FALSE);
        }

        bool isLooping() const override
        {
            return ma_sound_is_looping(m_sound.get()) == MA_TRUE;
        }

        void setLoopPoints(float startSeconds, float endSeconds) const override
        {
            const uint32_t rate = sampleRate();
            ma_data_source* dataSource = ma_sound_get_data_source(m_sound.get());
            if (rate == 0 || dataSource == nullptr || endSeconds <= startSeconds) return;

            ma_data_source_set_loop_point_in_pcm_frames(
                dataSource,
                static_cast<ma_uint64>(startSeconds * static_cast<float>(rate)),
                static_cast<ma_uint64>(endSeconds * static_cast<float>(rate)));
        }

        void seek(float seconds) const override
        {
            ma_sound_seek_to_second(m_sound.get(), seconds);
        }

        float currentTime() const override
        {
            float cursor = 0.0f;
            ma_sound_get_cursor_in_seconds(m_sound.get(), &cursor);
            return cursor;
        }

        float duration() const override
        {
            float length = 0.0f;
            ma_sound_get_length_in_seconds(m_sound.get(), &length);
            return length;
        }

        void setFade(float fromVolume, float toVolume, float milliseconds) const override
        {
            ma_sound_set_fade_in_milliseconds(m_sound.get(), fromVolume, toVolume, static_cast<ma_uint64>(milliseconds));
        }

        uint32_t sampleRate() const override
        {
            ma_uint32 rate = 0;
            ma_sound_get_data_format(m_sound.get(), nullptr, nullptr, &rate, nullptr, 0);
            return rate;
        }

        uint32_t channels() const override
        {
            ma_uint32 channelCount = 0;
            ma_sound_get_data_format(m_sound.get(), nullptr, &channelCount, nullptr, nullptr, 0);
            return channelCount;
        }

        uint64_t frameCount() const override
        {
            ma_uint64 length = 0;
            ma_sound_get_length_in_pcm_frames(m_sound.get(), &length);
            return length;
        }

        void onEnded(std::function<void()> callback) const override
        {
            if (!m_endedState) {
                m_endedState = std::make_shared<SoundEndedState>();
                m_audio->watchEndedState(m_endedState);
                ma_sound_set_end_callback(m_sound.get(), &MiniaudioSoundImpl::onSoundEnd, m_endedState.get());
            }
            m_endedState->callback = std::move(callback);
        }

        std::shared_ptr<SoundImpl> clone() const override;

    private:
        // Audio-thread side of onEnded(): only flips a flag, AudioComponent::update()
        // invokes the user callback on the main thread.
        static void onSoundEnd(void* userData, ma_sound*)
        {
            static_cast<SoundEndedState*>(userData)->pending.store(true, std::memory_order_relaxed);
        }

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
    };
} // namespace p5cpp

namespace p5cpp
{
    namespace
    {
        std::unique_ptr<SoundImpl> makeMemorySound(ma_engine* engine, AudioComponent* audio, std::shared_ptr<const std::vector<uint8_t>> encodedData)
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

            return std::make_unique<MiniaudioSoundImpl>(engine, audio, std::move(sound), std::move(decoder), std::move(encodedData));
        }

        std::unique_ptr<SoundImpl> makePcmSound(ma_engine* engine, AudioComponent* audio, std::shared_ptr<const std::vector<float>> pcmData, uint32_t sampleRate, uint32_t channels)
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

            return std::make_unique<MiniaudioSoundImpl>(engine, audio, std::move(sound), std::move(audioBuffer), std::move(pcmData), sampleRate, channels);
        }
    } // namespace

    std::shared_ptr<SoundImpl> MiniaudioSoundImpl::clone() const
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

        return std::make_shared<MiniaudioSoundImpl>(m_engine, m_audio, std::move(sound), false);
    }

    std::unique_ptr<SoundImpl> loadSoundImpl(AudioComponent& audio, const std::filesystem::path& soundFilePath, bool stream)
    {
        const ma_uint32 flags = stream ? MA_SOUND_FLAG_STREAM : MA_SOUND_FLAG_DECODE;

        auto sound = std::make_unique<ma_sound>();
        const ma_result result = ma_sound_init_from_file(audio.getEngine(), soundFilePath.string().c_str(), flags, nullptr, nullptr, sound.get());
        if (result != MA_SUCCESS) {
            error("Failed to load sound: " + soundFilePath.string());
            return nullptr;
        }

        return std::make_unique<MiniaudioSoundImpl>(audio.getEngine(), &audio, std::move(sound), stream);
    }

    std::unique_ptr<SoundImpl> loadSoundImpl(AudioComponent& audio, std::span<const uint8_t> soundData)
    {
        // ma_decoder_init_memory() does not copy the bytes — it reads from them on
        // demand as the sound plays, so we keep our own copy alive for the sound's
        // lifetime rather than relying on the caller's buffer to outlive playback.
        auto data = std::make_shared<const std::vector<uint8_t>>(soundData.begin(), soundData.end());
        return makeMemorySound(audio.getEngine(), &audio, std::move(data));
    }

    std::unique_ptr<SoundImpl> createSoundImpl(AudioComponent& audio, std::span<const float> samples, uint32_t sampleRate, uint32_t channels)
    {
        auto data = std::make_shared<const std::vector<float>>(samples.begin(), samples.end());
        return makePcmSound(audio.getEngine(), &audio, std::move(data), sampleRate, channels);
    }
} // namespace p5cpp

namespace p5cpp
{
    Sound::Sound()
        : impl(nullptr)
    {
    }

    Sound::Sound(std::unique_ptr<SoundImpl> impl)
        : impl(std::move(impl))
    {
    }

    Sound::Sound(std::shared_ptr<SoundImpl> impl)
        : impl(std::move(impl))
    {
    }

    bool Sound::isValid() const { return impl != nullptr; }
    Sound::operator bool() const { return impl != nullptr; }

    void Sound::play() const { if (impl) impl->play(); }
    void Sound::stop() const { if (impl) impl->stop(); }
    void Sound::pause() const { if (impl) impl->pause(); }
    bool Sound::isPlaying() const { return impl ? impl->isPlaying() : false; }
    SoundStatus Sound::status() const { return impl ? impl->status() : SoundStatus::stopped; }

    void Sound::setVolume(float volume) const { if (impl) impl->setVolume(volume); }
    float Sound::getVolume() const { return impl ? impl->getVolume() : 0.0f; }
    void Sound::setPan(float pan) const { if (impl) impl->setPan(pan); }
    float Sound::getPan() const { return impl ? impl->getPan() : 0.0f; }
    void Sound::setRate(float rate) const { if (impl) impl->setRate(rate); }
    float Sound::getRate() const { return impl ? impl->getRate() : 0.0f; }
    void Sound::setLoop(bool loop) const { if (impl) impl->setLoop(loop); }
    bool Sound::isLooping() const { return impl ? impl->isLooping() : false; }
    void Sound::setLoopPoints(float startSeconds, float endSeconds) const { if (impl) impl->setLoopPoints(startSeconds, endSeconds); }

    void Sound::seek(float seconds) const { if (impl) impl->seek(seconds); }
    float Sound::currentTime() const { return impl ? impl->currentTime() : 0.0f; }
    float Sound::duration() const { return impl ? impl->duration() : 0.0f; }

    void Sound::setFade(float fromVolume, float toVolume, float milliseconds) const { if (impl) impl->setFade(fromVolume, toVolume, milliseconds); }

    void Sound::fadeIn(float milliseconds) const
    {
        if (!impl) return;
        impl->setFade(0.0f, 1.0f, milliseconds);
        impl->play();
    }

    void Sound::fadeOut(float milliseconds) const
    {
        if (impl) impl->setFade(-1.0f, 0.0f, milliseconds);
    }

    uint32_t Sound::sampleRate() const { return impl ? impl->sampleRate() : 0; }
    uint32_t Sound::channels() const { return impl ? impl->channels() : 0; }
    uint64_t Sound::frameCount() const { return impl ? impl->frameCount() : 0; }

    void Sound::onEnded(std::function<void()> callback) const { if (impl) impl->onEnded(std::move(callback)); }

    Sound Sound::clone() const { return impl ? Sound(impl->clone()) : Sound(); }
} // namespace p5cpp
