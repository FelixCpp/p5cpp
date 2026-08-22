#include <p5cpp_audio/p5cpp_audio.hpp>
#include <p5cpp_audio/audio_engine.hpp>

namespace p5::audio
{
    namespace
    {
        inline static std::unique_ptr<AudioEngine> s_audioEngine;
    }

    class AudioPlugin : public Plugin
    {
        void setup(Context& context, const Next& next) override
        {
            s_audioEngine = AudioEngine::create();
            if (s_audioEngine == nullptr) {
                throw std::runtime_error("Failed to create audio engine (miniaudio initialization failed)");
            }

            context.provide(s_audioEngine.get());

            next();
        }

        void event([[maybe_unused]] Context& context, const Next& next, [[maybe_unused]] const WindowEvent& event) override
        {
            next();
        }

        void draw([[maybe_unused]] Context& context, const Next& next) override
        {
            next();
        }

        void destroy(Context& context, const Next& next) override
        {
            next();

            context.remove<AudioEngine>();
            s_audioEngine.reset();
        }
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
