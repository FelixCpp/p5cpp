#include <p5cpp_audio/p5cpp_audio.hpp>
#include <p5cpp_audio/audio_engine.hpp>

namespace p5::audio
{
    Sound loadSoundFromFile(const std::filesystem::path& filepath) { return getAudioEngine().loadSoundFromFile(filepath); }
    Sound loadSoundFromMemory(const std::span<const uint8_t> data) { return getAudioEngine().loadSoundFromMemory(data); }
    Sound createSoundAlias(const Sound& sound) { return getAudioEngine().createSoundAlias(sound); }
    void playSound(const Sound& sound) { getAudioEngine().playSound(sound); }
    void playSoundOverlapped(const Sound& sound) { getAudioEngine().playSoundOverlapped(sound); }
    void pauseSound(const Sound& sound) { getAudioEngine().pauseSound(sound); }
    void resumeSound(const Sound& sound) { getAudioEngine().resumeSound(sound); }
    void stopSound(const Sound& sound) { getAudioEngine().stopSound(sound); }
    bool isSoundPlaying(const Sound& sound) { return getAudioEngine().isSoundPlaying(sound); }
    PlaybackState getSoundPlaybackState(const Sound& sound) { return getAudioEngine().getSoundPlaybackState(sound); }
    bool isSoundValid(const Sound& sound) { return sound.resource != nullptr; }
    void setSoundVolume(const Sound& sound, const float volume) { getAudioEngine().setSoundVolume(sound, volume); }
    void setSoundPitch(const Sound& sound, const float pitch) { getAudioEngine().setSoundPitch(sound, pitch); }
    void setSoundPan(const Sound& sound, const float pan) { getAudioEngine().setSoundPan(sound, pan); }
    void setSoundLoop(const Sound& sound, const bool loop) { getAudioEngine().setSoundLoop(sound, loop); }
    bool isSoundLooping(const Sound& sound) { return getAudioEngine().isSoundLooping(sound); }
    void seekSound(const Sound& sound, const float seconds) { getAudioEngine().seekSound(sound, seconds); }
    float getSoundTimePlayed(const Sound& sound) { return getAudioEngine().getSoundTimePlayed(sound); }
    float getSoundTimeLength(const Sound& sound) { return getAudioEngine().getSoundTimeLength(sound); }
    MixedAudioProcessorHandle attachMixedAudioProcessor(MixedAudioProcessor processor)
    {
        return getAudioEngine().attachMixedAudioProcessor(std::move(processor));
    }

    void detachMixedAudioProcessor(const MixedAudioProcessorHandle handle) { getAudioEngine().detachMixedAudioProcessor(handle); }

    SoundProcessorHandle attachSoundProcessor(const Sound& sound, SoundProcessor processor)
    {
        return getAudioEngine().attachSoundProcessor(sound, std::move(processor));
    }

    void detachSoundProcessor(const Sound& sound, const SoundProcessorHandle handle) { getAudioEngine().detachSoundProcessor(sound, handle); }

    void setMasterVolume(const float volume) { getAudioEngine().setMasterVolume(volume); }
    float getMasterVolume() { return getAudioEngine().getMasterVolume(); }
} // namespace p5::audio
