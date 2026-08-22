#include <p5cpp_audio/sound_resource.hpp>

namespace p5::audio
{
    SoundResource::~SoundResource()
    {
        if (initialized) {
            ma_sound_uninit(&sound);
        }
    }
} // namespace p5::audio
