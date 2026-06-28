#include <filesystem>
#include <p5cpp.hpp>

#include <miniaudio.h>

namespace p5cpp
{
    class MiniAudioSound
    {
    public:
        static std::unique_ptr<MiniAudioSound> loadSoundFromFile(const std::filesystem::path& filepath)
        {
            ma_decoder decoder;
            if (ma_decoder_init_file("my_file.wav", NULL, &decoder) != MA_SUCCESS) {
                return nullptr;
            }

            ma_device_config deviceConfig;
            deviceConfig = ma_device_config_init(ma_device_type_playback);
            deviceConfig.playback.format = decoder.outputFormat;
            deviceConfig.playback.channels = decoder.outputChannels;
            deviceConfig.sampleRate = decoder.outputSampleRate;
            deviceConfig.dataCallback = data_callback;
            deviceConfig.pUserData = &decoder;

            ma_device device;
            if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
                ma_decoder_uninit(&decoder);
                return nullptr;
            }

            if (ma_device_start(&device) != MA_SUCCESS) {
                ma_device_uninit(&device);
                ma_decoder_uninit(&decoder);
                return nullptr;
            }

            return std::unique_ptr<MiniAudioSound>(new MiniAudioSound(std::move(decoder), std::move(deviceConfig), std::move(device)));
        }

        ~MiniAudioSound()
        {
            ma_device_uninit(&device);
            ma_decoder_uninit(&decoder);
        }

    private:
        explicit MiniAudioSound(ma_decoder decoder, ma_device_config deviceConfig, ma_device device)
            : decoder(std::move(decoder)), deviceConfig(std::move(deviceConfig)), device(std::move(device))
        {
        }

        static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
        {
            ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
            ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, NULL);
        }

        void setVolume(float volume)
        {
        }

    private:
        ma_decoder decoder;
        ma_device_config deviceConfig;
        ma_device device;
    };
} // namespace p5cpp

namespace p5cpp
{
    class SoundRegistry
    {
    public:
        Sound loadSound(const std::filesystem::path& filepath)
        {
            const size_t soundId = sounds.size();
            sounds.push_back(MiniAudioSound::loadSoundFromFile(filepath));
            return Sound {.id = soundId};
        }

    private:
        std::vector<std::unique_ptr<MiniAudioSound>> sounds;
    };
} // namespace p5cpp

namespace p5cpp
{
    Sound loadSound(const std::filesystem::path& filepath)
    {
    }

    void playSound(Sound sound)
    {
    }

    void setSoundVolume(Sound sound, float volume)
    {
    }
} // namespace p5cpp
