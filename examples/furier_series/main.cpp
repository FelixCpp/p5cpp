#include <algorithm>
#include <p5cpp.hpp>
#include <span>

#include "data.hpp"

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

class Scratchpad
{
public:
    std::shared_ptr<Framebuffer> framebuffer;
    std::vector<int2> points;

    explicit Scratchpad(int width, int height)
        : framebuffer(createFramebuffer(width, height)),
          points()
    {
    }

    void paintAt(int mouseX, int mouseY)
    {
        points.push_back(int2 {.x = mouseX, .y = mouseY});
    }

    void erase()
    {
        points.clear();
    }

    void show()
    {
        pushCanvas(framebuffer);
        {
            background(21);

            noFill();
            stroke(255);
            strokeWeight(2.0f);

            beginShape();
            for (const int2& point : points) {
                vertex(static_cast<float>(point.x), static_cast<float>(point.y));
            }
            endShape(ShapeType::lineStrip, false);
        }
        popCanvas();
    }
};

class ScratchpadController
{
public:
    explicit ScratchpadController(std::unique_ptr<Scratchpad> scratchpad)
        : scratchpad(std::move(scratchpad))
    {
    }

    void open()
    {
        isScratchpadOpen = true;
    }

    std::vector<int2> close()
    {
        std::vector<int2> points = scratchpad->points;
        scratchpad->erase();
        isScratchpadOpen = false;
        return points;
    }

    bool isOpen() const
    {
        return isScratchpadOpen;
    }

    void beginDrawing()
    {
        if (isScratchpadOpen) {
            isDrawing = true;
        }
    }

    void endDrawing()
    {
        if (isScratchpadOpen) {
            isDrawing = false;
        }
    }

    void update()
    {
        if (isDrawing) {
            const auto [width, height] = scratchpad->framebuffer->getSize();
            const int left = static_cast<float>(getWidth() - width) / 2;
            const int top = static_cast<float>(getHeight() - height) / 2;
            const int mouseX = getMouseX() - left;
            const int mouseY = getMouseY() - top;
            const bool isMouseInScratchPad = mouseX >= 0 and mouseX < width and mouseY >= 0 and mouseY < height;

            if (isMouseInScratchPad) {
                scratchpad->paintAt(mouseX, mouseY);
            }
        }
    }

    void show()
    {
        if (isScratchpadOpen) {
            scratchpad->show();

            const auto [width, height] = scratchpad->framebuffer->getSize();
            const float left = (static_cast<float>(getWidth()) - static_cast<float>(width)) * 0.5f;
            const float top = (static_cast<float>(getHeight()) - static_cast<float>(height)) * 0.5f;

            image(scratchpad->framebuffer->getTextureId(), left, top, static_cast<float>(width), static_cast<float>(height));

            noFill();
            stroke(255);
            strokeWeight(2.0f);
            rect(left, top, static_cast<float>(width), static_cast<float>(height), 25.0f, 25.0f);
        }
    }

private:
    std::unique_ptr<Scratchpad> scratchpad;
    bool isDrawing = false;
    bool isScratchpadOpen = false;
};

class DrawingMotor
{
public:
    explicit DrawingMotor(float2 position, float rotation)
        : position(std::move(position)), rotation(rotation)
    {
    }

    void setSignal(std::span<const float> values)
    {
        signal = dft(values);

        std::sort(signal.begin(), signal.end(), [](const Sample& a, const Sample& b) {
            return a.amplitude > b.amplitude;
        });
    }

    void update(float animationTime)
    {
        this->animationTime = animationTime;
    }

    void resetAnimation()
    {
        animationTime = 0.0f;
    }

    float2 getPosition() const
    {
        float x = position.x;
        float y = position.y;

        for (const Sample& sample : signal) {
            const float freq = sample.frequency;
            const float radius = sample.amplitude;
            const float phase = sample.phase;

            x += radius * std::cos(freq * animationTime + phase + rotation);
            y += radius * std::sin(freq * animationTime + phase + rotation);
        }

        return {x, y};
    }

    void show()
    {
        float x = position.x;
        float y = position.y;

        for (const Sample& sample : signal) {
            float prevx = x;
            float prevy = y;

            const float freq = sample.frequency;
            const float radius = sample.amplitude;
            const float phase = sample.phase;

            x += radius * std::cos(freq * animationTime + phase + rotation);
            y += radius * std::sin(freq * animationTime + phase + rotation);

            stroke(255, 100);
            noFill();
            circle(prevx, prevy, radius * 2.0f);

            stroke(255);
            line(prevx, prevy, x, y);
        }
    }

private:
    std::vector<Sample> signal;
    float2 position;
    float animationTime = 0.0f;
    float rotation;
};

class DrawingMachine
{
public:
    explicit DrawingMachine(std::span<const float2> points)
        : motorX({500.0f, 100.0f}, 0.0f),
          motorY({100.0f, 500.0f}, HALF_PI)
    {
        consumeDrawing(points);
    }

    void consumeDrawing(std::span<const float2> points)
    {
        output.clear();
        signalLength = points.size();

        std::vector<float> scratchX(signalLength);
        std::vector<float> scratchY(signalLength);
        for (size_t i = 0; i < signalLength; ++i) {
            const float2& point = points[i];
            scratchX[i] = static_cast<float>(point.x);
            scratchY[i] = static_cast<float>(point.y);
        }

        motorX.setSignal(scratchX);
        motorY.setSignal(scratchY);
    }

    void update(float deltaTime)
    {
        animationTime += TWO_PI / static_cast<float>(signalLength);
        if (animationTime > TWO_PI) {
            output.clear();
            animationTime = 0.0f;
        }

        motorX.update(animationTime);
        motorY.update(animationTime);

        const float2 px = motorX.getPosition();
        const float2 py = motorY.getPosition();
        drawingPoint = {px.x, py.y};

        output.push_back(drawingPoint);
    }

    void show()
    {
        motorX.show();
        motorY.show();

        stroke(255);
        strokeWeight(2.0f);
        beginShape();
        for (int i = 0; i < output.size(); ++i) {
            const float2& p = output.at(i);
            vertex(p.x, p.y);
        }
        endShape(ShapeType::lineStrip, false);

        const float2 px = motorX.getPosition();
        const float2 py = motorY.getPosition();

        line(px.x, px.y, drawingPoint.x, drawingPoint.y);
        line(py.x, py.y, drawingPoint.x, drawingPoint.y);
    }

private:
    std::vector<Sample> signalX;
    std::vector<Sample> signalY;
    std::vector<float2> output;

    size_t signalLength = 0;
    bool isAnimating = false;
    float animationTime = 0.0f;
    float2 drawingPoint = {0.0f, 0.0f};

    DrawingMotor motorX;
    DrawingMotor motorY;
};

struct Demo : Sketch
{
    std::unique_ptr<DrawingMachine> drawingMachine;
    std::unique_ptr<ScratchpadController> scratchPadController;

    void setup() override
    {
        setWindowSize(900, 900);
        frameRate(60);

        scratchPadController = std::make_unique<ScratchpadController>(
            std::make_unique<Scratchpad>(450, 450)
        );

        std::vector<float2> transformedData;
        for (int i = 0; i < data.size(); i += 5) {
            transformedData.push_back(float2 {data[i].x, data[i].y});
        }

        drawingMachine = std::make_unique<DrawingMachine>(transformedData);
    }

    void event(const WindowEvent& event) override
    {
        if (event.type == EventType::keyPress) {
            if (event.keyEvent.key == Key::enter) {
                if (scratchPadController->isOpen()) {
                    const std::vector<int2> scratchPoints = scratchPadController->close();
                    std::vector<float2> scratchPointsFloat(scratchPoints.size());
                    for (size_t i = 0; i < scratchPoints.size(); ++i) {
                        const int2& point = scratchPoints.at(i);
                        scratchPointsFloat[i] = {static_cast<float>(point.x), static_cast<float>(point.y)};
                    }
                    drawingMachine->consumeDrawing(scratchPointsFloat);
                } else {
                    scratchPadController->open();
                }
            }
        }

        if (event.type == EventType::mousePress) {
            if (event.mouseButton.button == MouseButton::left) {
                scratchPadController->beginDrawing();
            }
        }

        if (event.type == EventType::mouseRelease) {
            if (event.mouseButton.button == MouseButton::left) {
                scratchPadController->endDrawing();
            }
        }
    }

    void draw() override
    {
        background(31);

        const float deltaTime = getDeltaTime();

        if (not scratchPadController->isOpen()) {
            drawingMachine->update(deltaTime);
        }

        drawingMachine->show();

        scratchPadController->update();
        scratchPadController->show();
    }
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<Demo>();
    }
} // namespace p5cpp
