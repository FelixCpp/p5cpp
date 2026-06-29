#pragma once

#include <filesystem>
#include <memory>

namespace p5cpp
{
    typedef float SoundDuration;
    typedef float SoundVolume;
    typedef float SoundPitch;
    typedef float SoundPan;
    typedef bool SoundLooping;

    struct AudioBuffer
    {
        virtual ~AudioBuffer() = default;

        virtual void play() = 0;
        virtual bool isPlaying() const = 0;

        virtual void stop() = 0;

        virtual void pause() = 0;
        virtual bool isPaused() const = 0;

        virtual void resume() = 0;

        virtual void setVolume(SoundVolume volume) = 0;
        virtual SoundVolume getVolume() const = 0;

        virtual void setPan(SoundPan pan) = 0;
        virtual SoundPan getPan() const = 0;

        virtual void setPitch(SoundPitch pitch) = 0;
        virtual SoundPitch getPitch() const = 0;

        virtual void setLooping(SoundLooping loop) = 0;
        virtual SoundLooping isLooping() const = 0;

        virtual SoundDuration getDuration() const = 0;
        virtual void seek(SoundDuration position) = 0;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct AudioDevice
    {
        virtual ~AudioDevice() = default;

        static std::unique_ptr<AudioDevice> getDefaultDevice();

        virtual uint32_t getSampleRate() const = 0;
        virtual uint32_t getChannelCount() const = 0;
        virtual void* getNativeHandle() = 0;
    };

    struct AudioEngine
    {
        virtual ~AudioEngine() = default;

        static std::unique_ptr<AudioEngine> create(AudioDevice& device);

        virtual std::unique_ptr<AudioBuffer> loadSoundBuffer(const std::filesystem::path& filepath) = 0;
        virtual std::unique_ptr<AudioBuffer> loadMusicBuffer(const std::filesystem::path& filepath) = 0;

        virtual void setMasterVolume(SoundVolume volume) = 0;
        virtual SoundVolume getMasterVolume() const = 0;
    };
} // namespace p5cpp
