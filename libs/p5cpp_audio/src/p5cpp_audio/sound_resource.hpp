#pragma once

#include <filesystem>
#include <memory>
#include <span>
#include <cstdint>

#include <miniaudio.h>

namespace p5::audio
{
    class SoundResource
    {
    public:
        static std::unique_ptr<SoundResource> loadFromFile(ma_engine& engine, const std::filesystem::path& filepath);
        static std::unique_ptr<SoundResource> loadFromMemory(ma_engine& engine, std::span<const uint8_t> data);
        static std::unique_ptr<SoundResource> createAlias(ma_engine& engine, const SoundResource& source);

        ~SoundResource();

        SoundResource(const SoundResource&) = delete;
        SoundResource& operator=(const SoundResource&) = delete;
        SoundResource(SoundResource&&) = delete;
        SoundResource& operator=(SoundResource&&) = delete;

        void play();
        void pause();
        void resume();
        void stop();
        bool isPlaying() const;
        bool isAtEnd() const;
        uint64_t getCursorInFrames() const;

        void setVolume(float volume);
        void setPitch(float pitch);
        void setPan(float pan);

    private:
        explicit SoundResource();

        ma_sound m_sound;
        std::unique_ptr<ma_decoder> m_decoder;
    };
} // namespace p5::audio
