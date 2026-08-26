#pragma once

#include <p5cpp_audio/p5cpp_audio.hpp>

#include <filesystem>
#include <memory>
#include <span>
#include <cstdint>

#include <miniaudio.h>

namespace p5::audio
{
    struct SoundProcessorNode; // defined in sound_resource.cpp -- wraps the spliced-in ma_node

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

        void setLooping(bool loop);
        bool isLooping() const;

        void seek(float seconds);
        float getTimePlayed() const;
        float getTimeLength() const;

        SoundProcessorHandle attachProcessor(SoundProcessor processor);
        void detachProcessor(SoundProcessorHandle handle);

    private:
        explicit SoundResource();

        ma_sound m_sound;
        std::unique_ptr<ma_decoder> m_decoder;
        ma_engine* m_engine = nullptr;
        std::unique_ptr<SoundProcessorNode> m_processorNode; // lazily created on first attachProcessor
    };
} // namespace p5::audio
