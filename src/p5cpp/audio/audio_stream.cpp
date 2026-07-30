#include <p5cpp/audio/audio_stream.hpp>
#include <p5cpp/audio/audio_component.hpp>
#include <p5cpp/application/logging.hpp>

#include <miniaudio.h>

#include <algorithm>
#include <vector>

namespace p5cpp
{
    namespace
    {
        // A pull-based miniaudio data source that produces samples by invoking the
        // user's AudioStreamCallback. It has no length or cursor — it plays until
        // the sound is stopped.
        struct CallbackDataSource
        {
            ma_data_source_base base; // must be the first member (miniaudio casts to it)
            AudioStreamCallback callback;
            ma_uint32 channels = 0;
            ma_uint32 sampleRate = 0;
        };

        ma_result callbackDataSourceRead(ma_data_source* pDataSource, void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead)
        {
            auto* dataSource = reinterpret_cast<CallbackDataSource*>(pDataSource);

            if (pFramesOut != nullptr) {
                float* out = static_cast<float*>(pFramesOut);
                const size_t sampleCount = static_cast<size_t>(frameCount) * dataSource->channels;
                std::fill_n(out, sampleCount, 0.0f);
                if (dataSource->callback) {
                    dataSource->callback(std::span<float>(out, sampleCount), dataSource->channels);
                }
            }

            if (pFramesRead != nullptr) {
                *pFramesRead = frameCount;
            }
            return MA_SUCCESS;
        }

        ma_result callbackDataSourceSeek(ma_data_source*, ma_uint64)
        {
            return MA_SUCCESS;
        }

        ma_result callbackDataSourceGetDataFormat(ma_data_source* pDataSource, ma_format* pFormat, ma_uint32* pChannels, ma_uint32* pSampleRate, ma_channel* pChannelMap, size_t channelMapCap)
        {
            auto* dataSource = reinterpret_cast<CallbackDataSource*>(pDataSource);

            if (pFormat != nullptr) *pFormat = ma_format_f32;
            if (pChannels != nullptr) *pChannels = dataSource->channels;
            if (pSampleRate != nullptr) *pSampleRate = dataSource->sampleRate;
            if (pChannelMap != nullptr) {
                ma_channel_map_init_standard(ma_standard_channel_map_default, pChannelMap, channelMapCap, dataSource->channels);
            }
            return MA_SUCCESS;
        }

        constexpr ma_data_source_vtable callbackDataSourceVtable = {
            callbackDataSourceRead,
            callbackDataSourceSeek,
            callbackDataSourceGetDataFormat,
            nullptr, // onGetCursor
            nullptr, // onGetLength
            nullptr, // onSetLooping
            0,
        };
    } // namespace
} // namespace p5cpp

namespace p5cpp::detail
{
    struct AudioStreamResource
    {
        AudioStreamResource(std::unique_ptr<CallbackDataSource> dataSource, std::unique_ptr<ma_sound> sound)
            : m_dataSource(std::move(dataSource)),
              m_sound(std::move(sound))
        {
        }

        ~AudioStreamResource()
        {
            // Sound first: after ma_sound_uninit() the audio thread no longer reads
            // from the data source (and thus no longer invokes the user callback).
            ma_sound_uninit(m_sound.get());
            ma_data_source_uninit(&m_dataSource->base);
        }

        void play() const { ma_sound_start(m_sound.get()); }
        void stop() const { ma_sound_stop(m_sound.get()); }
        void pause() const { ma_sound_stop(m_sound.get()); }
        bool isPlaying() const { return ma_sound_is_playing(m_sound.get()) == MA_TRUE; }

        void setVolume(float volume) const { ma_sound_set_volume(m_sound.get(), volume); }
        float getVolume() const { return ma_sound_get_volume(m_sound.get()); }
        void setPan(float pan) const { ma_sound_set_pan(m_sound.get(), pan); }
        float getPan() const { return ma_sound_get_pan(m_sound.get()); }
        void setRate(float rate) const { ma_sound_set_pitch(m_sound.get(), rate); }
        float getRate() const { return ma_sound_get_pitch(m_sound.get()); }

        std::unique_ptr<CallbackDataSource> m_dataSource;
        std::unique_ptr<ma_sound> m_sound;
    };
} // namespace p5cpp::detail

namespace p5cpp
{
    AudioStream createAudioStream(uint32_t sampleRate, uint32_t channels, AudioStreamCallback callback)
    {
        if (sampleRate == 0 || channels == 0) {
            error("createAudioStream: sampleRate and channels must be non-zero");
            return AudioStream();
        }

        auto dataSource = std::make_unique<CallbackDataSource>();
        dataSource->callback = std::move(callback);
        dataSource->channels = channels;
        dataSource->sampleRate = sampleRate;

        ma_data_source_config config = ma_data_source_config_init();
        config.vtable = &callbackDataSourceVtable;
        if (ma_data_source_init(&config, &dataSource->base) != MA_SUCCESS) {
            error("createAudioStream: failed to initialize data source");
            return AudioStream();
        }

        AudioComponent& audio = getAudioComponent();

        auto sound = std::make_unique<ma_sound>();
        if (ma_sound_init_from_data_source(audio.getEngine(), dataSource.get(), 0, nullptr, sound.get()) != MA_SUCCESS) {
            ma_data_source_uninit(&dataSource->base);
            error("createAudioStream: failed to create stream sound");
            return AudioStream();
        }

        AudioStream stream;
        stream.sampleRate = sampleRate;
        stream.channels = channels;
        stream.resource = std::make_shared<detail::AudioStreamResource>(std::move(dataSource), std::move(sound));
        return stream;
    }
} // namespace p5cpp

namespace p5cpp
{
    namespace
    {
        // Drains an initialized decoder into an AudioSamples buffer and uninits it.
        // Chunked reads because some formats (e.g. mp3) cannot report their length
        // up front.
        AudioSamples readAllFrames(ma_decoder& decoder)
        {
            AudioSamples result;

            ma_format format = ma_format_unknown;
            ma_uint32 channels = 0;
            ma_uint32 sampleRate = 0;
            ma_decoder_get_data_format(&decoder, &format, &channels, &sampleRate, nullptr, 0);
            if (channels == 0 || sampleRate == 0) {
                ma_decoder_uninit(&decoder);
                error("loadAudioSamples: could not determine audio format");
                return result;
            }

            constexpr ma_uint64 chunkFrames = 4096;
            std::vector<float> chunk(chunkFrames * channels);

            std::vector<float> samples;
            for (;;) {
                ma_uint64 framesRead = 0;
                const ma_result readResult = ma_decoder_read_pcm_frames(&decoder, chunk.data(), chunkFrames, &framesRead);
                samples.insert(samples.end(), chunk.begin(), chunk.begin() + static_cast<size_t>(framesRead * channels));
                if (readResult != MA_SUCCESS || framesRead < chunkFrames) break;
            }
            ma_decoder_uninit(&decoder);

            result.samples = std::move(samples);
            result.sampleRate = sampleRate;
            result.channels = channels;
            return result;
        }
    } // namespace

    AudioSamples loadAudioSamples(const std::filesystem::path& soundFilePath)
    {
        const ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0); // native channels/rate

        ma_decoder decoder;
        if (ma_decoder_init_file(soundFilePath.string().c_str(), &config, &decoder) != MA_SUCCESS) {
            error("loadAudioSamples: failed to open " + soundFilePath.string());
            return AudioSamples {};
        }
        return readAllFrames(decoder);
    }

    AudioSamples loadAudioSamples(std::span<const uint8_t> soundData)
    {
        const ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0); // native channels/rate

        // Unlike loadSound(), no copy of the bytes is needed: the data is fully
        // decoded before this function returns.
        ma_decoder decoder;
        if (ma_decoder_init_memory(soundData.data(), soundData.size(), &config, &decoder) != MA_SUCCESS) {
            error("loadAudioSamples: failed to decode in-memory sound data");
            return AudioSamples {};
        }
        return readAllFrames(decoder);
    }
} // namespace p5cpp

namespace p5cpp
{
    bool isAudioStreamValid(const AudioStream& stream) { return stream.resource != nullptr; }

    void playAudioStream(const AudioStream& stream) { if (stream.resource) stream.resource->play(); }
    void stopAudioStream(const AudioStream& stream) { if (stream.resource) stream.resource->stop(); }
    void pauseAudioStream(const AudioStream& stream) { if (stream.resource) stream.resource->pause(); }
    bool isPlaying(const AudioStream& stream) { return stream.resource ? stream.resource->isPlaying() : false; }

    void setAudioStreamVolume(const AudioStream& stream, float volume) { if (stream.resource) stream.resource->setVolume(volume); }
    float getAudioStreamVolume(const AudioStream& stream) { return stream.resource ? stream.resource->getVolume() : 0.0f; }
    void setAudioStreamPan(const AudioStream& stream, float pan) { if (stream.resource) stream.resource->setPan(pan); }
    float getAudioStreamPan(const AudioStream& stream) { return stream.resource ? stream.resource->getPan() : 0.0f; }
    void setAudioStreamRate(const AudioStream& stream, float rate) { if (stream.resource) stream.resource->setRate(rate); }
    float getAudioStreamRate(const AudioStream& stream) { return stream.resource ? stream.resource->getRate() : 0.0f; }
} // namespace p5cpp
