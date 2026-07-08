#include <p5cpp/audio/audio_module.hpp>
#include <p5cpp/application/app_context.hpp>
#include <p5cpp/application/logging.hpp>

namespace p5cpp
{
    void AudioModule::setup(AppContext& context, Next next)
    {
        info("AudioModule setup");
        context.registerService(&m_audioComponent);
        next();
    }

    void AudioModule::draw(AppContext& context, Next next)
    {
        m_audioComponent.update();
        next();
    }

    void AudioModule::destroy(AppContext& context, Next next)
    {
        next();
        context.unregisterService<AudioComponent>();
    }
} // namespace p5cpp
