#include <p5cpp_audio/p5cpp_audio.hpp>
#include <p5cpp_audio/audio_engine.hpp>

namespace p5::audio
{
    bool Sound::isValid() const { return resource != nullptr; }

    void Sound::play() { getAudioEngine().play(*this); }
    void Sound::playOverlapped() { getAudioEngine().playOverlapped(*this); }
    void Sound::pause() { getAudioEngine().pause(*this); }
    void Sound::resume() { getAudioEngine().resume(*this); }
    void Sound::stop() { getAudioEngine().stop(*this); }
    bool Sound::isPlaying() const { return getAudioEngine().isPlaying(*this); }
    PlaybackState Sound::getPlaybackState() const { return getAudioEngine().getPlaybackState(*this); }

    void Sound::setVolume(const float volume) { getAudioEngine().setVolume(*this, volume); }
    void Sound::setPitch(const float pitch) { getAudioEngine().setPitch(*this, pitch); }
    void Sound::setPan(const float pan) { getAudioEngine().setPan(*this, pan); }

    void Sound::setLoop(const bool loop) { getAudioEngine().setLoop(*this, loop); }
    bool Sound::isLooping() const { return getAudioEngine().isLooping(*this); }

    void Sound::seek(const float seconds) { getAudioEngine().seek(*this, seconds); }
    float Sound::getTimePlayed() const { return getAudioEngine().getTimePlayed(*this); }
    float Sound::getTimeLength() const { return getAudioEngine().getTimeLength(*this); }

    Sound Sound::createAlias() const { return getAudioEngine().createAlias(*this); }

    SoundProcessorHandle Sound::attachProcessor(SoundProcessor processor) { return getAudioEngine().attachProcessor(*this, std::move(processor)); }
    void Sound::detachProcessor(const SoundProcessorHandle handle) { getAudioEngine().detachProcessor(*this, handle); }

    std::optional<Sound> loadSound(const std::filesystem::path& filepath) { return getAudioEngine().loadSound(filepath); }
    std::optional<Sound> loadSound(const std::span<const uint8_t> data) { return getAudioEngine().loadSound(data); }

    void setMasterVolume(const float volume) { getAudioEngine().setMasterVolume(volume); }
    float getMasterVolume() { return getAudioEngine().getMasterVolume(); }

    MixedAudioProcessorHandle attachMixedAudioProcessor(MixedAudioProcessor processor)
    {
        return getAudioEngine().attachMixedAudioProcessor(std::move(processor));
    }

    void detachMixedAudioProcessor(const MixedAudioProcessorHandle handle) { getAudioEngine().detachMixedAudioProcessor(handle); }
} // namespace p5::audio
