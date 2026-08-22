#include <p5cpp_audio/p5cpp_audio.hpp>
#include <p5cpp_audio/audio_engine.hpp>
#include <p5cpp_audio/sound_resource.hpp>

namespace p5::audio
{
    Sound loadSound(const std::filesystem::path& path, const Bus& bus) { return {.resource = getAudioEngine().loadSoundResource(path, bus)}; }
    void playSound(const Sound& sound) { getAudioEngine().playSound(sound); }
    void stopSound(const Sound& sound) { getAudioEngine().stopSound(sound); }
    void pauseSound(const Sound& sound) { getAudioEngine().pauseSound(sound); }
    void resumeSound(const Sound& sound) { getAudioEngine().resumeSound(sound); }
    void setSoundVolume(const Sound& sound, float volume) { getAudioEngine().setSoundVolume(sound, volume); }
    void setSoundPan(const Sound& sound, float pan) { getAudioEngine().setSoundPan(sound, pan); }
    void setSoundPitch(const Sound& sound, float pitch) { getAudioEngine().setSoundPitch(sound, pitch); }
} // namespace p5::audio
