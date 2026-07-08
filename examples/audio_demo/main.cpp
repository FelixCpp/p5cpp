// audio_demo — minimal p5cpp audio API showcase.
//
// Synthesizes a short sine-wave tone as an in-memory WAV buffer (no bundled
// binary assets, matching how the rest of examples/ stays self-contained)
// and exercises the loadSound(std::span<const uint8_t>) overload. Pressing
// space or clicking plays the tone; the waveform ring buffer and RMS
// amplitude reported by the engine drive a live oscilloscope + pulsing
// circle, demonstrating both playback and audio analysis.

#include <p5cpp/p5cpp.hpp>

#include <cmath>
#include <cstdint>
#include <vector>

using namespace p5cpp;

namespace
{
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

    // Builds a mono 16-bit PCM WAV file in memory: a sine tone with a short
    // fade in/out envelope so playback doesn't click at the start/end.
    std::vector<uint8_t> makeSineWaveTone(float frequencyHz, float durationSeconds, uint32_t sampleRate)
    {
        const uint32_t frameCount = static_cast<uint32_t>(durationSeconds * static_cast<float>(sampleRate));
        const uint32_t dataSize = frameCount * sizeof(int16_t);

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

        const float fadeSeconds = 0.05f;
        const uint32_t fadeFrames = static_cast<uint32_t>(fadeSeconds * static_cast<float>(sampleRate));

        for (uint32_t i = 0; i < frameCount; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(sampleRate);
            float envelope = 1.0f;
            if (i < fadeFrames) envelope = static_cast<float>(i) / static_cast<float>(fadeFrames);
            if (i >= frameCount - fadeFrames) envelope = static_cast<float>(frameCount - i) / static_cast<float>(fadeFrames);

            const float sample = std::sin(TWO_PI * frequencyHz * t) * envelope * 0.8f;
            const int16_t pcm = static_cast<int16_t>(sample * 32767.0f);

            bytes.push_back(static_cast<uint8_t>(static_cast<uint16_t>(pcm) & 0xFF));
            bytes.push_back(static_cast<uint8_t>((static_cast<uint16_t>(pcm) >> 8) & 0xFF));
        }

        return bytes;
    }
} // namespace

struct AudioDemoSketch : Sketch
{
    Sound tone;

    void setup() override
    {
        setWindowSize(800, 500);
        setWindowTitle("p5cpp - Audio Demo");

        const std::vector<uint8_t> wavBytes = makeSineWaveTone(440.0f, 1.2f, 44100);
        tone = loadSound(std::span<const uint8_t> {wavBytes});
    }

    void event(const WindowEvent& e) override
    {
        if (e.type == EventType::keyPress && e.keyEvent.key == Key::space) {
            playSound(tone);
        }

        if (e.type == EventType::mousePress && e.mouseButton.button == MouseButton::left) {
            playSound(tone);
        }
    }

    void draw() override
    {
        background(15, 15, 20, 255);

        const float w = static_cast<float>(getLogicalWidth());
        const float h = static_cast<float>(getLogicalHeight());
        const float midY = h * 0.5f;

        const float amplitude = getAudioAmplitude();
        const std::span<const float> waveform = getAudioWaveform();

        noFill();
        stroke(80, 200, 255, 220);
        strokeWeight(2.0f);
        beginShape();
        for (size_t i = 0; i < waveform.size(); ++i) {
            const float x = w * static_cast<float>(i) / static_cast<float>(waveform.size() - 1);
            const float y = midY + waveform[i] * midY * 0.9f;
            vertex(x, y);
        }
        endShape(ShapeType::lineStrip, false);

        noStroke();
        fill(255, 120, 80, 200);
        circle(w * 0.5f, h * 0.82f, 30.0f + amplitude * 260.0f);

        fill(230);
        textAlign(TextAlign::topLeft);
        textSize(18.0f);
        text("p5cpp Audio Demo - Space or click to play a synthesized tone", 20.0f, 20.0f);
        text(isPlaying(tone) ? "Status: playing" : "Status: idle", 20.0f, 48.0f);
    }
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<AudioDemoSketch>();
    }
} // namespace p5cpp
