#include <p5cpp_audio/sound_resource.hpp>

#include <p5cpp/p5cpp.hpp>

namespace p5::audio
{
    std::unique_ptr<SoundResource> SoundResource::loadFromFile(ma_engine& engine, const std::filesystem::path& filepath)
    {
        std::unique_ptr<SoundResource> resource {new SoundResource()};
        const std::string fp = filepath.string();

        if (ma_sound_init_from_file(&engine, fp.c_str(), 0, nullptr, nullptr, &resource->m_sound) != MA_SUCCESS) {
            error("Failed to load sound from filepath \"{}\"", fp);
            return nullptr;
        }

        return resource;
    }

    std::unique_ptr<SoundResource> SoundResource::loadFromMemory(ma_engine& engine, const std::span<const uint8_t> data)
    {
        std::unique_ptr<SoundResource> resource {new SoundResource()};
        resource->m_decoder = std::make_unique<ma_decoder>();

        if (ma_decoder_init_memory(data.data(), data.size(), nullptr, resource->m_decoder.get()) != MA_SUCCESS) {
            error("Failed to decode sound from memory");
            return nullptr;
        }

        if (ma_sound_init_from_data_source(&engine, resource->m_decoder.get(), 0, nullptr, &resource->m_sound) != MA_SUCCESS) {
            error("Failed to load sound from memory");
            ma_decoder_uninit(resource->m_decoder.get());
            return nullptr;
        }

        return resource;
    }

    std::unique_ptr<SoundResource> SoundResource::createAlias(ma_engine& engine, const SoundResource& source)
    {
        std::unique_ptr<SoundResource> resource {new SoundResource()};

        if (ma_sound_init_copy(&engine, &source.m_sound, 0, nullptr, &resource->m_sound) != MA_SUCCESS) {
            error("Failed to create sound alias");
            return nullptr;
        }

        return resource;
    }

    SoundResource::~SoundResource()
    {
        ma_sound_uninit(&m_sound);

        if (m_decoder) {
            ma_decoder_uninit(m_decoder.get());
        }
    }

    void SoundResource::play()
    {
        ma_sound_start(&m_sound);
    }

    void SoundResource::pause()
    {
        ma_sound_stop(&m_sound);
    }

    void SoundResource::resume()
    {
        play();
    }

    void SoundResource::stop()
    {
        ma_sound_stop(&m_sound);
        ma_sound_seek_to_pcm_frame(&m_sound, 0);
    }

    bool SoundResource::isPlaying() const
    {
        return ma_sound_is_playing(&m_sound);
    }

    bool SoundResource::isAtEnd() const
    {
        return ma_sound_at_end(&m_sound);
    }

    uint64_t SoundResource::getCursorInFrames() const
    {
        ma_uint64 cursor = 0;
        ma_sound_get_cursor_in_pcm_frames(&m_sound, &cursor);

        return cursor;
    }

    void SoundResource::setVolume(const float volume)
    {
        ma_sound_set_volume(&m_sound, volume);
    }

    void SoundResource::setPitch(const float pitch)
    {
        ma_sound_set_pitch(&m_sound, pitch);
    }

    void SoundResource::setPan(const float pan)
    {
        ma_sound_set_pan(&m_sound, pan);
    }

    SoundResource::SoundResource() = default;
} // namespace p5::audio
