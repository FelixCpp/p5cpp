#pragma once

#include <p5cpp/p5cpp.hpp>

#include <memory>
#include <filesystem>
#include <cstdint>

namespace p5::audio
{
    struct BusResource;
    struct Bus
    {
        std::shared_ptr<BusResource> resource;
    };

    // parent == Bus{} (the default) attaches the new bus directly to the master bus.
    Bus createBus(const Bus& parent = {});

    void setBusVolume(const Bus& bus, float volume);
    void setBusPan(const Bus& bus, float pan);
    void setBusPitch(const Bus& bus, float pitch);
} // namespace p5::audio

namespace p5::audio
{
    struct SoundResource;
    struct Sound
    {
        std::shared_ptr<SoundResource> resource;
    };

    // bus == Bus{} (the default) routes the sound directly to the master bus.
    Sound loadSound(const std::filesystem::path& path, const Bus& bus = {});
    void playSound(const Sound& sound);
    void stopSound(const Sound& sound);
    void pauseSound(const Sound& sound);
    void resumeSound(const Sound& sound);

    void setSoundVolume(const Sound& sound, float volume);
    void setSoundPan(const Sound& sound, float pan);
    void setSoundPitch(const Sound& sound, float pitch);
} // namespace p5::audio

namespace p5::audio
{
    // Effects are separate concrete types (one per underlying miniaudio node type) rather than a
    // single polymorphic/variant "Effect" -- same taste as Texture/Shader/Framebuffer elsewhere in
    // this codebase: plain structs, no shared base. They only unify at the AudioNode/connect()
    // layer below, via their own asNode() overload.
    struct LowpassFilterResource;
    struct LowpassFilter
    {
        std::shared_ptr<LowpassFilterResource> resource;
    };

    LowpassFilter createLowpassFilter(float cutoffFrequencyHz, uint32_t order = 2);

    struct DelayResource;
    struct Delay
    {
        std::shared_ptr<DelayResource> resource;
    };

    Delay createDelay(uint32_t delayInFrames, float decay);
    void setDelayWet(const Delay& delay, float wet);
    void setDelayDry(const Delay& delay, float dry);
    void setDelayFeedback(const Delay& delay, float decay);

    struct ReverbResource;
    struct Reverb
    {
        std::shared_ptr<ReverbResource> resource;
    };

    Reverb createReverb();

    // Two input buses: bus 0 is the source/carrier, bus 1 is the excite/modulator (must be mono).
    // addEffect(bus, vocoder) / asNode(vocoder) only address bus 0 (the common case) -- wire the
    // excite input directly via connect(asNode(exciteBus), asNode(vocoder), 0, 1).
    struct VocoderResource;
    struct Vocoder
    {
        std::shared_ptr<VocoderResource> resource;
    };

    Vocoder createVocoder(uint32_t bands = 16, uint32_t filtersPerBand = 6);
} // namespace p5::audio

namespace p5::audio
{
    // Sits inline in the graph like any other effect (bus -> analyzer -> next bus/master via
    // addEffect()+connect()) and passes audio through unchanged, computing a running level as a
    // side effect. getLevel()/getPeakLevel() are safe to poll from draw() every frame.
    struct AnalyzerResource;
    struct Analyzer
    {
        std::shared_ptr<AnalyzerResource> resource;
    };

    Analyzer createAnalyzer();
    float getLevel(const Analyzer& analyzer);     // RMS over the most recently processed block.
    float getPeakLevel(const Analyzer& analyzer); // Peak sample magnitude over the same block.
} // namespace p5::audio

namespace p5::audio
{
    // Non-owning handle to a node in the audio graph (Sound, Bus, Effect, or Analyzer). Kept as an
    // opaque void* here rather than a miniaudio ma_node* so this public header stays miniaudio-free;
    // audio_node.cpp casts it back internally. Every graph participant gets an asNode() overload,
    // so connect()/disconnect() stay generic instead of a combinatorial set of connectXToY()
    // functions per type pairing.
    struct AudioNode
    {
        void* node = nullptr;
    };

    AudioNode asNode(const Sound& sound);
    AudioNode asNode(const Bus& bus);
    AudioNode asNode(const LowpassFilter& filter);
    AudioNode asNode(const Delay& delay);
    AudioNode asNode(const Reverb& reverb);
    AudioNode asNode(const Vocoder& vocoder);
    AudioNode asNode(const Analyzer& analyzer);
    AudioNode asNode(AudioNode node); // identity -- lets connect()/disconnect() below accept an
                                       // already-resolved AudioNode (e.g. getMasterBus()) alongside
                                       // Sound/Bus/Effect/Analyzer without a special case for it.

    AudioNode getMasterBus();

    void connect(AudioNode from, AudioNode to, uint32_t outputBus = 0, uint32_t inputBus = 0);
    void disconnect(AudioNode from, uint32_t outputBus = 0);

    // Generic overloads so callers don't have to sprinkle asNode() everywhere: connect(bus, reverb)
    // instead of connect(asNode(bus), asNode(reverb)). asNode() itself stays public as an escape
    // hatch (e.g. to stash an AudioNode for later), but everyday wiring code shouldn't need it.
    template <typename From, typename To> void connect(const From& from, const To& to, uint32_t outputBus = 0, uint32_t inputBus = 0)
    {
        connect(asNode(from), asNode(to), outputBus, inputBus);
    }

    template <typename From> void disconnect(const From& from, uint32_t outputBus = 0) { disconnect(asNode(from), outputBus); }

    // Sugar for connect(bus, effect): routes the bus's output into the effect's input. The
    // effect's own output still needs its own connect() onward (to the master bus or another bus)
    // -- addEffect() only wires the one edge it names, no implicit auto-chaining.
    template <typename Effect> void addEffect(const Bus& bus, const Effect& effect) { connect(bus, effect); }
} // namespace p5::audio

namespace p5::audio
{
    std::unique_ptr<Plugin> createAudioPlugin();
} // namespace p5::audio
