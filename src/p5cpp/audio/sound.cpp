#include <p5cpp/audio/sound_impl.hpp>
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
        explicit MiniaudioSoundImpl(std::unique_ptr<ma_sound> sound)
            : m_sound(std::move(sound))
        {
        }

        // Memory-backed: ma_sound_init_from_data_source() only stores a pointer to the
        // decoder we hand it — it does not take ownership or copy the decoder's backing
        // bytes. Both must outlive the sound (it streams/decodes from them on demand for
        // as long as it plays), so this impl owns and outlives them.
        MiniaudioSoundImpl(std::unique_ptr<ma_sound> sound, std::unique_ptr<ma_decoder> decoder, std::vector<uint8_t> data)
            : m_sound(std::move(sound)),
              m_decoder(std::move(decoder)),
              m_data(std::move(data))
        {
        }

        ~MiniaudioSoundImpl() override
        {
            ma_sound_uninit(m_sound.get());
            if (m_decoder) {
                ma_decoder_uninit(m_decoder.get());
            }
        }

        void play() const override
        {
            ma_sound_start(m_sound.get());
        }

        void stop() const override
        {
            ma_sound_stop(m_sound.get());
            ma_sound_seek_to_pcm_frame(m_sound.get(), 0);
        }

        void pause() const override
        {
            ma_sound_stop(m_sound.get());
        }

        bool isPlaying() const override
        {
            return ma_sound_is_playing(m_sound.get()) == MA_TRUE;
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

    private:
        std::unique_ptr<ma_sound> m_sound;
        std::unique_ptr<ma_decoder> m_decoder;
        std::vector<uint8_t> m_data;
    };
} // namespace p5cpp

namespace p5cpp
{
    std::unique_ptr<SoundImpl> loadSoundImpl(ma_engine* engine, const std::filesystem::path& soundFilePath)
    {
        auto sound = std::make_unique<ma_sound>();
        const ma_result result = ma_sound_init_from_file(engine, soundFilePath.string().c_str(), MA_SOUND_FLAG_DECODE, nullptr, nullptr, sound.get());
        if (result != MA_SUCCESS) {
            error("Failed to load sound: " + soundFilePath.string());
            return nullptr;
        }

        return std::make_unique<MiniaudioSoundImpl>(std::move(sound));
    }

    std::unique_ptr<SoundImpl> loadSoundImpl(ma_engine* engine, std::span<const uint8_t> soundData)
    {
        // ma_decoder_init_memory() does not copy soundData — it reads from it on demand
        // as the sound plays, so we keep our own copy alive for the sound's lifetime
        // rather than relying on the caller's buffer to outlive playback.
        std::vector<uint8_t> data(soundData.begin(), soundData.end());

        auto decoder = std::make_unique<ma_decoder>();
        const ma_result decoderResult = ma_decoder_init_memory(data.data(), data.size(), nullptr, decoder.get());
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

        return std::make_unique<MiniaudioSoundImpl>(std::move(sound), std::move(decoder), std::move(data));
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

    void Sound::play() const { impl->play(); }
    void Sound::stop() const { impl->stop(); }
    void Sound::pause() const { impl->pause(); }
    bool Sound::isPlaying() const { return impl->isPlaying(); }

    void Sound::setVolume(float volume) const { impl->setVolume(volume); }
    float Sound::getVolume() const { return impl->getVolume(); }
    void Sound::setPan(float pan) const { impl->setPan(pan); }
    float Sound::getPan() const { return impl->getPan(); }
    void Sound::setRate(float rate) const { impl->setRate(rate); }
    float Sound::getRate() const { return impl->getRate(); }
    void Sound::setLoop(bool loop) const { impl->setLoop(loop); }
    bool Sound::isLooping() const { return impl->isLooping(); }
} // namespace p5cpp
