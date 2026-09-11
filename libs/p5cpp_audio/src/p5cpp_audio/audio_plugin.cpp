#include <p5cpp_audio/p5cpp_audio.hpp>
#include <p5cpp_audio/audio_engine.hpp>

#include <miniaudio.h>

namespace p5::audio
{
    namespace
    {
        std::unique_ptr<AudioEngine> s_audioEngine;
    }

    class AudioPlugin : public Plugin
    {
    public:
        void setup(const Next& next) override
        {
            s_audioEngine = AudioEngine::create();
            provideDependency(s_audioEngine.get());

            next();
        }

        void event(const Next& next, [[maybe_unused]] const WindowEvent& event) override
        {
            next();
        }

        void draw(const Next& next) override
        {
            s_audioEngine->pruneFinishedOverlaps();

            next();
        }

        void destroy(const Next& next) override
        {
            next();

            removeDependency<AudioEngine>();
            s_audioEngine.reset();
        }

    private:
    };
} // namespace p5::audio

namespace p5::audio
{
    AudioEngine& getAudioEngine()
    {
        return *s_audioEngine;
    }
} // namespace p5::audio

namespace p5::audio
{
    std::unique_ptr<Plugin> createAudioPlugin()
    {
        return std::make_unique<AudioPlugin>();
    }
} // namespace p5::audio
