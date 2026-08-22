#include <p5cpp/p5cpp.hpp>
#include <p5cpp_audio/p5cpp_audio.hpp>

using namespace p5;
using namespace p5::audio;

struct AudioVisualizer : Sketch
{
    Sound sound;
    Bus bus;
    Reverb reverb;
    Delay delay;

    void setup() override
    {
        setWindowSize(800, 600);

        bus = createBus();
        delay = createDelay(100, 0.5f);
        reverb = createReverb();
        addEffect(bus, delay);
        addEffect(bus, reverb);
        connect(reverb, getMasterBus());
        connect(delay, getMasterBus());

        sound = loadSound("sound.mp3", bus);
        playSound(sound);
    }

    void draw() override
    {
        background(rgba(255));
    }

    std::vector<std::unique_ptr<Plugin>> plugins() override
    {
        std::vector<std::unique_ptr<Plugin>> plugins;
        plugins.push_back(createAudioPlugin());
        return plugins;
    }
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<AudioVisualizer>();
}
