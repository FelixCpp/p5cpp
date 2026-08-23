#include <p5cpp/p5cpp.hpp>
#include <p5cpp_audio/p5cpp_audio.hpp>

#include <vector>

using namespace p5;
using namespace p5::audio;

struct AudioVisualizer : Sketch
{
    Sound sound = loadSoundFromFile("sound.mp3");
    Sound alias = createSoundAlias(sound);

    void setup() override
    {
        setWindowSize(800, 600);

        setSoundVolume(sound, 1.0f);
        setSoundVolume(alias, 0.4f);
    }

    void event(const WindowEvent& event) override
    {
    }

    void draw() override
    {
        background(rgba(255));

        if (isKeyPressed(Key::A)) {
            playSound(sound);
        }

        if (isKeyPressed(Key::B)) {
            playSound(alias);
        }
    }
};

SketchSpec p5::createSpec()
{
    return {
        .plugins = [] {
            std::vector<std::unique_ptr<Plugin>> plugins;
            plugins.push_back(createAudioPlugin());
            return plugins;
        },
        .sketch = [] {
            return std::make_unique<AudioVisualizer>();
        },
    };
}
