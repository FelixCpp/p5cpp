#include <p5cpp.hpp>
#include "audio.hpp"

#include <miniaudio.h>

namespace p5cpp
{
    class MiniaudioAudioBuffer : public AudioBuffer
    {
    public:
        explicit MiniaudioAudioBuffer(ma_engine* parent, ma_sound sound)
            : parent(parent), sound(sound)
        {
        }

        ~MiniaudioAudioBuffer()
        {
            ma_sound_uninit(&sound);
        }

        void play() override
        {
            ma_sound_seek_to_pcm_frame(&sound, 0); // Reset the sound to the beginning before playing.
            ma_sound_start(&sound);
        }

        bool isPlaying() const override
        {
            return ma_sound_is_playing(&sound) != 0;
        }

        void stop() override
        {
            ma_sound_stop(&sound);
        }

        void pause() override
        {
            ma_sound_stop(&sound); // Miniaudio does not have a pause function, so we stop the sound instead.
        }

        bool isPaused() const override
        {
            return not ma_sound_is_playing(&sound) and not ma_sound_at_end(&sound);
        }

        void resume() override
        {
            ma_sound_start(&sound); // Miniaudio does not have a resume function, so we start the sound instead.
        }

        void setVolume(SoundVolume volume) override
        {
            ma_sound_set_volume(&sound, volume);
        }

        SoundVolume getVolume() const override
        {
            return SoundVolume {ma_sound_get_volume(&sound)};
        }

        void setPan(SoundPan pan) override
        {
            ma_sound_set_pan(&sound, pan);
        }

        SoundPan getPan() const override
        {
            return SoundPan {ma_sound_get_pan(&sound)};
        }

        void setPitch(SoundPitch pitch) override
        {
            ma_sound_set_pitch(&sound, pitch);
        }

        SoundPitch getPitch() const override
        {
            return SoundPitch {ma_sound_get_pitch(&sound)};
        }

        void setLooping(SoundLooping loop) override
        {
            ma_sound_set_looping(&sound, loop ? MA_TRUE : MA_FALSE);
        }

        SoundLooping isLooping() const override
        {
            return SoundLooping {ma_sound_is_looping(&sound) != MA_FALSE};
        }

        SoundDuration getDuration() const override
        {
            float durationInSeconds = 0.0f;
            ma_sound_get_length_in_seconds(&sound, &durationInSeconds);
            return SoundDuration {durationInSeconds};
        }

        void seek(SoundDuration position) override
        {
            const ma_uint32 sampleRate = ma_engine_get_sample_rate(parent);
            const ma_uint64 frameCount = static_cast<ma_uint64>(position * sampleRate);
            ma_sound_seek_to_pcm_frame(&sound, frameCount);
        }

    private:
        ma_engine* parent;
        ma_sound sound;
    };
} // namespace p5cpp

namespace p5cpp
{
    class MiniaudioAudioDevice : public AudioDevice
    {
    public:
        explicit MiniaudioAudioDevice(ma_device device, const ma_device_config& config)
            : device(std::move(device)), config(config)
        {
        }

        ~MiniaudioAudioDevice() override
        {
            ma_device_uninit(&device);
        }

        uint32_t getSampleRate() const override
        {
            return config.sampleRate;
        }

        uint32_t getChannelCount() const override
        {
            return config.playback.channels;
        }

        void* getNativeHandle() override
        {
            return &device;
        }

    private:
        ma_device device;
        ma_device_config config;
    };
} // namespace p5cpp

namespace p5cpp
{
    std::unique_ptr<AudioDevice> AudioDevice::getDefaultDevice()
    {
        ma_device defaultDevice;
        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        ma_device_init(nullptr, &config, &defaultDevice);
        ma_device_start(&defaultDevice);

        return std::make_unique<MiniaudioAudioDevice>(std::move(defaultDevice), std::move(config));
    }
} // namespace p5cpp

namespace p5cpp
{
    class MiniaudioAudioEngine : public AudioEngine
    {
    public:
        static std::unique_ptr<MiniaudioAudioEngine> create(AudioDevice& device)
        {
            ma_engine engine;

            ma_engine_config config = ma_engine_config_init();
            config.sampleRate = device.getSampleRate();
            config.channels = device.getChannelCount();
            config.pDevice = static_cast<ma_device*>(device.getNativeHandle());
            config.noAutoStart = MA_TRUE; // Prevent the engine from starting automatically
            config.noDevice = MA_FALSE;   // Ensure the engine uses the provided device

            if (ma_engine_init(&config, &engine) != MA_SUCCESS) {
                throw std::runtime_error("Failed to initialize audio engine");
            }

            return std::unique_ptr<MiniaudioAudioEngine>(new MiniaudioAudioEngine(std::move(engine)));
        }

        ~MiniaudioAudioEngine() override
        {
            ma_engine_uninit(&engine);
        }

        std::unique_ptr<AudioBuffer> loadSoundBuffer(const std::filesystem::path& filepath) override
        {
            ma_sound sound;
            if (ma_sound_init_from_file(&engine, filepath.string().c_str(), MA_SOUND_FLAG_DECODE, nullptr, nullptr, &sound) != MA_SUCCESS) {
                throw std::runtime_error("Failed to load sound from file: " + filepath.string());
            }

            return std::make_unique<MiniaudioAudioBuffer>(&engine, sound);
        }

        std::unique_ptr<AudioBuffer> loadMusicBuffer(const std::filesystem::path& filepath) override
        {
            ma_sound sound;
            if (ma_sound_init_from_file(&engine, filepath.string().c_str(), MA_SOUND_FLAG_STREAM, nullptr, nullptr, &sound) != MA_SUCCESS) {
                throw std::runtime_error("Failed to load sound from file: " + filepath.string());
            }

            return std::make_unique<MiniaudioAudioBuffer>(&engine, std::move(sound));
        }

        void setMasterVolume(SoundVolume volume) override
        {
            ma_engine_set_volume(&engine, volume);
        }

        SoundVolume getMasterVolume() const override
        {
            return SoundVolume {ma_engine_get_volume(&engine)};
        }

    private:
        explicit MiniaudioAudioEngine(ma_engine engine)
            : engine(std::move(engine))
        {
        }

        mutable ma_engine engine;
    };
} // namespace p5cpp

namespace p5cpp
{
    std::unique_ptr<AudioEngine> AudioEngine::create(AudioDevice& device)
    {
        return MiniaudioAudioEngine::create(device);
    }
} // namespace p5cpp
