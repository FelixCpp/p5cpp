#include <algorithm>
#include <p5cpp.hpp>
#include <span>

using namespace p5cpp;

struct Sample
{
    float real;
    float imag;
    float frequency;
    float amplitude;
    float phase;
};

static std::vector<Sample> dft(const std::span<const float>& input)
{
    const size_t N = input.size();
    std::vector<Sample> signal(N);

    for (size_t k = 0; k < N; ++k) {
        float real = 0.0f;
        float imag = 0.0f;

        for (size_t n = 0; n < N; ++n) {
            const float phi = (TWO_PI * k * n) / N;
            real += input[n] * std::cos(phi);
            imag -= input[n] * std::sin(phi);
        }

        real /= N;
        imag /= N;

        const float frequency = k;
        const float amplitude = std::sqrt(real * real + imag * imag);
        const float phase = std::atan2(imag, real);

        signal[k] = Sample {
            .real = real,
            .imag = imag,
            .frequency = frequency,
            .amplitude = amplitude,
            .phase = phase,
        };
    }

    return signal;
}

float2 cycle(float x, float y, float time, float rotation, const std::span<const Sample> signal)
{
    for (const Sample& sample : signal) {
        float prevx = x;
        float prevy = y;

        const float freq = sample.frequency;
        const float radius = sample.amplitude;
        const float phase = sample.phase;

        x += radius * std::cos(freq * time + phase + rotation);
        y += radius * std::sin(freq * time + phase + rotation);

        stroke(255, 100);
        noFill();
        circle(prevx, prevy, radius * 2.0f);

        stroke(255);
        line(prevx, prevy, x, y);
    }

    return {x, y};
}

struct Demo : Sketch
{
    std::vector<Sample> signalX;
    std::vector<Sample> signalY;
    std::vector<float2> output;
    float globalTime = 0.0f;

    std::shared_ptr<Framebuffer> scratchPad;
    std::vector<float2> scratchPadStrokes;
    bool isDrawing = false;
    bool isScratchPadOpen = false;

    void setup() override
    {
        setWindowSize(900, 900);
        frameRate(60);

        scratchPad = createFramebuffer(450, 450);

        std::vector<float> inputX;
        std::vector<float> inputY;
        for (size_t i = 0; i < 500; ++i) {
            const float x = 150.0f * noise(i / 50.0f);
            const float y = 150.0f * noise((i + 1000) / 50.0f);
            inputX.push_back(x);
            inputY.push_back(y);
        }

        signalX = dft(inputX);
        signalY = dft(inputY);

        pushCanvas(scratchPad);
        background(21);
        popCanvas();
    }

    void event(const WindowEvent& event) override
    {
        if (event.type == EventType::keyPress) {
            if (event.keyEvent.key == Key::enter) {
                isScratchPadOpen = !isScratchPadOpen;

                if (not isScratchPadOpen) {
                    std::vector<float> xs(scratchPadStrokes.size());
                    std::vector<float> ys(scratchPadStrokes.size());

                    for (size_t i = 0; i < scratchPadStrokes.size(); ++i) {
                        xs[i] = (scratchPadStrokes.at(i).x);
                        ys[i] = (scratchPadStrokes.at(i).y);
                    }

                    signalX = dft(xs);
                    signalY = dft(ys);
                    std::sort(signalX.begin(), signalX.end(), [](const Sample& a, const Sample& b) {
                        return a.amplitude > b.amplitude;
                    });

                    std::sort(signalY.begin(), signalY.end(), [](const Sample& a, const Sample& b) {
                        return a.amplitude > b.amplitude;
                    });

                    output.clear();
                }
            }
        }

        if (event.type == EventType::mousePress) {
            if (event.mouseButton.button == MouseButton::left) {
                isDrawing = true;
            }
        }

        if (event.type == EventType::mouseRelease) {
            if (event.mouseButton.button == MouseButton::left) {
                isDrawing = false;
            }
        }
    }

    void draw() override
    {
        globalTime += TWO_PI / signalX.size() * static_cast<float>(not isScratchPadOpen);

        background(31);

        const float2 machineX = cycle(500.0f, 100.0f, globalTime, 0.0f, signalX);
        const float2 machineY = cycle(100.0f, 500.0f, globalTime, HALF_PI, signalY);
        output.push_back({machineX.x, machineY.y});

        stroke(255);
        strokeWeight(2.0f);
        beginShape();
        for (int i = 0; i < output.size(); ++i) {
            const float2& p = output.at(i);
            vertex(p.x, p.y);
        }
        endShape(ShapeType::lineStrip, false);

        line(machineX.x, machineX.y, machineX.x, machineY.y);
        line(machineY.x, machineY.y, machineX.x, machineY.y);

        if (isScratchPadOpen) {
            const float scratchPadWidth = static_cast<float>(scratchPad->getSize().x);
            const float scratchPadHeight = static_cast<float>(scratchPad->getSize().y);

            const float scratchPadLeft = (static_cast<float>(getWidth()) - scratchPadWidth) * 0.5f;
            const float scratchPadTop = (static_cast<float>(getHeight()) - scratchPadHeight) * 0.5f;

            fill(0, 150);
            noStroke();
            rect(0.0f, 0.0f, static_cast<float>(getWidth()), static_cast<float>(getHeight()));

            textSize(48.0f);
            fill(255);
            textAlign(TextAlign::center);
            text("Draw Something", static_cast<float>(getWidth()) * 0.5f, scratchPadTop * 0.5f);

            pushCanvas(scratchPad);

            if (isDrawing) {
                // TODO(Felix): Handle drawing logic

                const int mouseX = getMouseX();
                const int mouseY = getMouseY();

                const bool isMouseInScratchPad = mouseX >= scratchPadLeft && mouseX <= scratchPadLeft + scratchPadWidth && mouseY >= scratchPadTop && mouseY <= scratchPadTop + scratchPadHeight;
                if (isMouseInScratchPad) {
                    const int cursorX = mouseX - scratchPadLeft;
                    const int cursorY = mouseY - scratchPadTop;

                    scratchPadStrokes.push_back({static_cast<float>(cursorX), static_cast<float>(cursorY)});
                }
            }

            stroke(255);
            strokeWeight(2.0f);
            beginShape();
            for (const float2& stroke : scratchPadStrokes) {
                vertex(stroke.x, stroke.y);
            }
            endShape(ShapeType::lineStrip, false);
            popCanvas();

            image(scratchPad->getTextureId(), scratchPadLeft, scratchPadTop, scratchPadWidth, scratchPadHeight);
            stroke(255);
            strokeWeight(2.0f);
            noFill();
            rect(scratchPadLeft, scratchPadTop, scratchPadWidth, scratchPadHeight, 20.0f, 20.0f);
        }
    }
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<Demo>();
    }
} // namespace p5cpp
