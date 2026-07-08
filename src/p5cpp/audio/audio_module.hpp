#pragma once

#include <p5cpp/application/module.hpp>
#include <p5cpp/audio/audio_component.hpp>

namespace p5cpp
{
    class AudioModule : public Module
    {
    public:
        void setup(AppContext& context, Next next) override;
        void draw(AppContext& context, Next next) override;
        void destroy(AppContext& context, Next next) override;

    private:
        AudioComponent m_audioComponent;
    };
} // namespace p5cpp
