#include <p5cpp_audio/sound_resource.hpp>

#include <p5cpp/p5cpp.hpp>

#include <mutex>
#include <utility>
#include <vector>
#include <cstring>

namespace p5::audio
{
    // A custom ma_node spliced in between a ma_sound and whatever it's attached to (the engine
    // endpoint by default). See "Writing custom nodes" in miniaudio.h for the pattern this follows.
    // `base` must stay the first member -- miniaudio casts a `ma_node*` straight back to this type.
    // Defined in p5::audio (not anonymously) to match the forward declaration in sound_resource.hpp.
    struct SoundProcessorNode
    {
        ma_node_base base;
        uint32_t channels = 0;
        std::mutex mutex;
        std::vector<std::pair<uint64_t, SoundProcessor>> processors;
        uint64_t nextId = 1;
    };

    namespace
    {
        void processSoundProcessorNode(ma_node* node, const float** framesIn, ma_uint32*, float** framesOut, ma_uint32* frameCountOut)
        {
            auto& self = *static_cast<SoundProcessorNode*>(node);
            const auto sampleCount = static_cast<size_t>(*frameCountOut) * self.channels;

            std::memcpy(framesOut[0], framesIn[0], sampleCount * sizeof(float));
            const std::span<float> frames {framesOut[0], sampleCount};

            std::lock_guard lock {self.mutex};
            for (const auto& [id, processor] : self.processors) {
                processor(frames, self.channels);
            }
        }

        const ma_node_vtable g_soundProcessorNodeVTable {
            .onProcess = &processSoundProcessorNode,
            .onGetRequiredInputFrameCount = nullptr,
            .inputBusCount = 1,
            .outputBusCount = 1,
            .flags = 0,
        };

        std::unique_ptr<SoundProcessorNode> createSoundProcessorNode(ma_engine& engine, ma_sound& sound)
        {
            auto node = std::make_unique<SoundProcessorNode>();
            node->channels = ma_engine_get_channels(&engine);

            const ma_uint32 inputChannels[1] {node->channels};
            const ma_uint32 outputChannels[1] {node->channels};

            ma_node_config config = ma_node_config_init();
            config.vtable = &g_soundProcessorNodeVTable;
            config.pInputChannels = inputChannels;
            config.pOutputChannels = outputChannels;

            if (ma_node_init(ma_engine_get_node_graph(&engine), &config, nullptr, &node->base) != MA_SUCCESS) {
                error("Failed to initialize sound processor node");
                return nullptr;
            }

            // Build from the end, like miniaudio's own "engine_effects" example: attach the node's
            // output to where the sound is currently attached (the endpoint, by default), then
            // rewire the sound's output onto the node. ma_node_attach_output_bus() moves an already
            // attached output bus, so the sound doesn't need to be detached first.
            ma_node_attach_output_bus(&node->base, 0, ma_engine_get_endpoint(&engine), 0);
            ma_node_attach_output_bus(&sound, 0, &node->base, 0);

            return node;
        }
    } // namespace

    std::unique_ptr<SoundResource> SoundResource::loadFromFile(ma_engine& engine, const std::filesystem::path& filepath)
    {
        std::unique_ptr<SoundResource> resource {new SoundResource()};
        resource->m_engine = &engine;
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
        resource->m_engine = &engine;
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
        resource->m_engine = &engine;

        if (ma_sound_init_copy(&engine, &source.m_sound, 0, nullptr, &resource->m_sound) != MA_SUCCESS) {
            error("Failed to create sound alias");
            return nullptr;
        }

        return resource;
    }

    SoundResource::~SoundResource()
    {
        ma_sound_uninit(&m_sound);

        if (m_processorNode) {
            ma_node_uninit(&m_processorNode->base, nullptr);
        }

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

    void SoundResource::setLooping(const bool loop)
    {
        ma_sound_set_looping(&m_sound, loop ? MA_TRUE : MA_FALSE);
    }

    bool SoundResource::isLooping() const
    {
        return ma_sound_is_looping(&m_sound);
    }

    void SoundResource::seek(const float seconds)
    {
        if (ma_sound_seek_to_second(&m_sound, seconds) != MA_SUCCESS) {
            error("Failed to seek sound to {}s", seconds);
        }
    }

    float SoundResource::getTimePlayed() const
    {
        float cursor = 0.0f;
        ma_sound_get_cursor_in_seconds(&m_sound, &cursor);

        return cursor;
    }

    float SoundResource::getTimeLength() const
    {
        float length = 0.0f;
        ma_sound_get_length_in_seconds(&m_sound, &length);

        return length;
    }

    SoundProcessorHandle SoundResource::attachProcessor(SoundProcessor processor)
    {
        if (not m_processorNode) {
            m_processorNode = createSoundProcessorNode(*m_engine, m_sound);

            if (not m_processorNode) {
                return SoundProcessorHandle {};
            }
        }

        std::lock_guard lock {m_processorNode->mutex};
        const uint64_t id = m_processorNode->nextId++;
        m_processorNode->processors.emplace_back(id, std::move(processor));

        return SoundProcessorHandle {.id = id};
    }

    void SoundResource::detachProcessor(const SoundProcessorHandle handle)
    {
        if (not m_processorNode) {
            return;
        }

        std::lock_guard lock {m_processorNode->mutex};
        std::erase_if(m_processorNode->processors, [handle](const auto& entry) {
            return entry.first == handle.id;
        });
    }

    SoundResource::SoundResource() = default;
} // namespace p5::audio
