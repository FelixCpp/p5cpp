// audio_playground — showcases the extended p5cpp audio API.
//
// Stays self-contained like audio_demo: all audio is synthesized at startup.
// A short melody is rendered to a WAV in memory and written to a temp file so
// loadMusic() can exercise the streaming path (seek bar, currentTime/duration,
// status, onEnded, fades, loop). A percussive blip built with createSound()
// from raw PCM samples demonstrates polyphony via playSoundMulti(), and a
// theremin-style sine synth driven by the mouse demonstrates createAudioStream()
// with a callback running on the audio thread.

#include <p5cpp/p5cpp.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <span>
#include <vector>

using namespace p5cpp;

namespace
{
    constexpr uint32_t SAMPLE_RATE = 44100;

    void appendUint32(std::vector<uint8_t>& bytes, uint32_t value)
    {
        bytes.push_back(static_cast<uint8_t>(value & 0xFF));
        bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        bytes.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        bytes.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    }

    void appendUint16(std::vector<uint8_t>& bytes, uint16_t value)
    {
        bytes.push_back(static_cast<uint8_t>(value & 0xFF));
        bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    }

    // Wraps mono float samples into a 16-bit PCM WAV file in memory.
    std::vector<uint8_t> makeWav(const std::vector<float>& samples, uint32_t sampleRate)
    {
        const uint32_t dataSize = static_cast<uint32_t>(samples.size() * sizeof(int16_t));

        std::vector<uint8_t> bytes;
        bytes.reserve(44 + dataSize);

        bytes.insert(bytes.end(), {'R', 'I', 'F', 'F'});
        appendUint32(bytes, 36 + dataSize);
        bytes.insert(bytes.end(), {'W', 'A', 'V', 'E'});

        bytes.insert(bytes.end(), {'f', 'm', 't', ' '});
        appendUint32(bytes, 16);
        appendUint16(bytes, 1); // PCM
        appendUint16(bytes, 1); // mono
        appendUint32(bytes, sampleRate);
        appendUint32(bytes, sampleRate * sizeof(int16_t));
        appendUint16(bytes, static_cast<uint16_t>(sizeof(int16_t)));
        appendUint16(bytes, 16);

        bytes.insert(bytes.end(), {'d', 'a', 't', 'a'});
        appendUint32(bytes, dataSize);

        for (const float sample : samples) {
            const auto pcm = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f);
            bytes.push_back(static_cast<uint8_t>(static_cast<uint16_t>(pcm) & 0xFF));
            bytes.push_back(static_cast<uint8_t>((static_cast<uint16_t>(pcm) >> 8) & 0xFF));
        }

        return bytes;
    }

    // A pentatonic arpeggio (~12 s) so seeking to a different position is
    // immediately audible.
    std::vector<float> makeMelodySamples()
    {
        constexpr float noteSeconds = 0.35f;
        constexpr int noteCount = 34;
        constexpr int scale[] = {0, 2, 4, 7, 9};

        const auto framesPerNote = static_cast<uint32_t>(noteSeconds * SAMPLE_RATE);
        const auto fadeFrames = static_cast<uint32_t>(0.015f * SAMPLE_RATE);

        std::vector<float> samples;
        samples.reserve(static_cast<size_t>(noteCount) * framesPerNote);

        for (int note = 0; note < noteCount; ++note) {
            const int semitone = scale[note % 5] + 12 * ((note / 5) % 2);
            const float frequency = 220.0f * std::pow(2.0f, static_cast<float>(semitone) / 12.0f);

            for (uint32_t i = 0; i < framesPerNote; ++i) {
                float envelope = 1.0f;
                if (i < fadeFrames) envelope = static_cast<float>(i) / static_cast<float>(fadeFrames);
                if (i >= framesPerNote - fadeFrames) envelope = static_cast<float>(framesPerNote - i) / static_cast<float>(fadeFrames);

                const float t = static_cast<float>(i) / static_cast<float>(SAMPLE_RATE);
                samples.push_back(std::sin(TWO_PI * frequency * t) * envelope * 0.5f);
            }
        }

        return samples;
    }

    // A short percussive blip: raw PCM for createSound().
    std::vector<float> makeBlipSamples()
    {
        const auto frameCount = static_cast<uint32_t>(0.15f * SAMPLE_RATE);

        std::vector<float> samples;
        samples.reserve(frameCount);

        for (uint32_t i = 0; i < frameCount; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(SAMPLE_RATE);
            samples.push_back(std::sin(TWO_PI * 880.0f * t) * std::exp(-t * 28.0f) * 0.7f);
        }

        return samples;
    }
} // namespace

struct AudioPlaygroundSketch : Sketch
{
    // Written by draw() on the main thread, read by the stream callback on the
    // audio thread; phase/gain are audio-thread-only.
    struct SynthState
    {
        std::atomic<float> frequency {440.0f};
        std::atomic<float> targetGain {0.0f};
        float phase = 0.0f;
        float gain = 0.0f;
    };

    Sound music;
    Sound blip;
    AudioStream synth;
    std::shared_ptr<SynthState> synthState;
    int endedCount = 0;
    bool isMouseDown = false;

    static constexpr float seekBarX = 20.0f;
    static constexpr float seekBarY = 150.0f;
    static constexpr float seekBarW = 620.0f;
    static constexpr float seekBarH = 18.0f;

    void setup() override
    {
        setWindowSize(900, 560);
        setWindowTitle("p5cpp - Audio Playground");

        // loadMusic() streams from disk, so render the melody to a temp file.
        const std::vector<uint8_t> wavBytes = makeWav(makeMelodySamples(), SAMPLE_RATE);
        const std::filesystem::path musicPath = std::filesystem::temp_directory_path() / "p5cpp_audio_playground.wav";
        std::ofstream(musicPath, std::ios::binary).write(reinterpret_cast<const char*>(wavBytes.data()), static_cast<std::streamsize>(wavBytes.size()));

        music = loadMusic(musicPath);
        onSoundEnded(music, [this] { ++endedCount; });

        const std::vector<float> blipSamples = makeBlipSamples();
        blip = createSound(blipSamples, SAMPLE_RATE, 1);

        synthState = std::make_shared<SynthState>();
        synth = createAudioStream(SAMPLE_RATE, 2, [state = synthState](std::span<float> frames, uint32_t channels) {
            const float target = state->targetGain.load(std::memory_order_relaxed);
            const float phaseStep = TWO_PI * state->frequency.load(std::memory_order_relaxed) / static_cast<float>(SAMPLE_RATE);

            for (size_t i = 0; i < frames.size(); i += channels) {
                state->gain += (target - state->gain) * 0.002f;
                const float sample = std::sin(state->phase) * state->gain * 0.3f;
                for (uint32_t c = 0; c < channels; ++c) {
                    frames[i + c] = sample;
                }
                state->phase += phaseStep;
                if (state->phase > TWO_PI) state->phase -= TWO_PI;
            }
        });
        playAudioStream(synth); // runs continuously; targetGain gates the tone
    }

    void event(const WindowEvent& e) override
    {
        if (e.type == EventType::keyPress) {
            switch (e.keyEvent.key) {
                case Key::escape: quit(); break;
                case Key::space: getSoundStatus(music) == SoundStatus::playing ? pauseSound(music) : playSound(music); break;
                case Key::s: stopSound(music); break;
                case Key::l: setSoundLoop(music, !isSoundLooping(music)); break;
                case Key::f: fadeInSound(music, 1500.0f); break;
                case Key::g: fadeOutSound(music, 1500.0f); break;
                case Key::m: playSoundMulti(blip); break;
                case Key::left: seekSound(music, std::max(0.0f, getSoundCurrentTime(music) - 2.0f)); break;
                case Key::right: seekSound(music, std::min(getSoundDuration(music), getSoundCurrentTime(music) + 2.0f)); break;
                case Key::t: synthState->targetGain.store(1.0f, std::memory_order_relaxed); break;
                default: break;
            }
        }

        if (e.type == EventType::keyRelease && e.keyEvent.key == Key::t) {
            synthState->targetGain.store(0.0f, std::memory_order_relaxed);
        }

        if (e.type == EventType::mousePress && e.mouseButton.button == MouseButton::left) {
            isMouseDown = true;
        }
        if (e.type == EventType::mouseRelease && e.mouseButton.button == MouseButton::left) {
            isMouseDown = false;
        }
    }

    void drawSeekBar()
    {
        const auto mx = static_cast<float>(getMouseX());
        const auto my = static_cast<float>(getMouseY());
        const bool hovering = mx >= seekBarX && mx <= seekBarX + seekBarW && my >= seekBarY && my <= seekBarY + seekBarH;

        if (isMouseDown && hovering && getSoundDuration(music) > 0.0f) {
            seekSound(music, (mx - seekBarX) / seekBarW * getSoundDuration(music));
        }

        const float progress = getSoundDuration(music) > 0.0f ? getSoundCurrentTime(music) / getSoundDuration(music) : 0.0f;

        noStroke();
        fill(80, 200, 255, 180);
        rect(seekBarX, seekBarY, seekBarW * progress, seekBarH);
        stroke(80, 200, 255, 220);
        strokeWeight(2.0f);
        noFill();
        rect(seekBarX, seekBarY, seekBarW, seekBarH);
    }

    void draw() override
    {
        background(15, 15, 20, 255);

        // Theremin: mouse Y controls the synth pitch.
        const float pitch01 = 1.0f - static_cast<float>(getMouseY()) / static_cast<float>(getLogicalHeight());
        synthState->frequency.store(110.0f * std::pow(8.0f, std::clamp(pitch01, 0.0f, 1.0f)), std::memory_order_relaxed);

        const float w = static_cast<float>(getLogicalWidth());
        const float h = static_cast<float>(getLogicalHeight());

        fill(230);
        noStroke();
        textAlign(TextAlign::topLeft);
        textSize(18.0f);
        text("p5cpp Audio Playground", 20.0f, 20.0f);
        textSize(14.0f);
        text("space: play/pause   s: stop   left/right: seek   l: loop   f/g: fade in/out", 20.0f, 52.0f);
        text("m: overlapping blips (playSoundMulti)   hold t: synth stream (pitch = mouse Y)   click bar: seek", 20.0f, 74.0f);

        const char* statusName = "stopped";
        if (getSoundStatus(music) == SoundStatus::playing) statusName = "playing";
        if (getSoundStatus(music) == SoundStatus::paused) statusName = "paused";

        char line[160];
        std::snprintf(line, sizeof(line), "music: %s%s   %.1fs / %.1fs   ended %d time(s)", statusName, isSoundLooping(music) ? " (loop)" : "", getSoundCurrentTime(music), getSoundDuration(music), endedCount);
        text(line, 20.0f, 110.0f);

        std::snprintf(line, sizeof(line), "%u Hz, %u channel(s), %llu frames", music.sampleRate, music.channels, static_cast<unsigned long long>(getSoundFrameCount(music)));
        text(line, 20.0f, 130.0f);

        drawSeekBar();

        // Oscilloscope of everything currently audible.
        const std::span<const float> waveform = getAudioWaveform();
        const float midY = h * 0.62f;
        noFill();
        stroke(230, 140, 80, 255);
        strokeWeight(2.0f);
        beginShape();
        for (size_t i = 0; i < waveform.size(); ++i) {
            const float x = w * static_cast<float>(i) / static_cast<float>(waveform.size() - 1);
            vertex(x, midY + waveform[i] * h * 0.3f);
        }
        endShape(ShapeType::lineStrip, false);

        noStroke();
        fill(255, 120, 80, 200);
        circle(w * 0.5f, h * 0.9f, 20.0f + getAudioAmplitude() * 220.0f);
    }
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<AudioPlaygroundSketch>();
    }
} // namespace p5cpp
