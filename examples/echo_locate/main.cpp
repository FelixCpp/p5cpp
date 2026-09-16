#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>

using namespace p5;
using namespace p5::animation;

inline static constexpr color_t SEGMENT_COLOR = rgba(255, 191, 30);
inline static constexpr color_t SEGMENT_BASE_COLOR = rgba(51, 51, 51);
inline static constexpr color_t SEGMENT_PREVIEW_COLOR = rgba(61, 61, 61, 150);

inline static constexpr color_t SWEEP_COLOR = rgba(135, 206, 235);
inline static constexpr color_t POP_FLASH_COLOR = rgba(255, 255, 255);

inline static constexpr float FLASH_DURATION = 1.0f;

struct LineSegment
{
    float2 start;
    float2 end;
    ValueTweenTransition<float> flash;
};

inline static void segment_flash(LineSegment& segment)
{
    segment.flash = valueTween(1.0f, 0.0f, FLASH_DURATION, &curves::easeOutExpo);
}

inline static void segment_update(LineSegment& segment, float deltaTime)
{
    segment.flash.advance(deltaTime);
}

inline static void segment_draw(const LineSegment& segment)
{
    const color_t segmentColor = lerpColor(SEGMENT_BASE_COLOR, SEGMENT_COLOR, segment.flash.value());

    strokeCap(StrokeCap::round);
    strokeWeight(2.0f);
    stroke(segmentColor);
    line(segment.start.x, segment.start.y, segment.end.x, segment.end.y);
}

struct Sweep
{
    ValueTweenTransition<float> radius;
    float2 emitterPosition;
    float previousRadius = 0.0f;
};

inline static bool sweep_done(const Sweep& sweep)
{
    return sweep.radius.isFinished();
}

inline static void sweep_update(Sweep& sweep, float deltaTime)
{
    if (sweep_done(sweep)) {
        return;
    }

    sweep.previousRadius = sweep.radius.value();
    sweep.radius.advance(deltaTime);
}

inline static void sweep_draw(const Sweep& sweep)
{
    if (sweep_done(sweep)) {
        return;
    }

    const float opacity = 1.0f - sweep.radius.progress();
    const color_t sweepColor = withOpacity(SWEEP_COLOR, opacity);

    noFill();
    stroke(sweepColor);
    strokeWeight(2.0f);
    circle(sweep.emitterPosition.x, sweep.emitterPosition.y, sweep.radius.value());
}

inline static bool sweep_reaches_segment(const Sweep& sweep, const LineSegment& segment)
{
    const float2 d = segment.end - segment.start;
    const float2 f = segment.start - sweep.emitterPosition;

    const float a = dot(d, d);
    const float b = 2.0f * dot(f, d);
    const float c = dot(f, f) - (sweep.radius.value() * sweep.radius.value());

    float discriminant = b * b - 4.0f * a * c;

    if (discriminant < 0) {
        return false; // No intersection
    }

    discriminant = sqrt(discriminant);
    const float t1 = (-b - discriminant) / (2.0f * a);
    const float t2 = (-b + discriminant) / (2.0f * a);

    return (t1 >= 0 && t1 <= 1) || (t2 >= 0 && t2 <= 1);
}

inline static constexpr float PULSE_MIN_RADIUS = 3.0f;
inline static constexpr float PULSE_MAX_RADIUS = 7.0f;
inline static constexpr float PULSE_POP_RADIUS = 11.0f;
inline static constexpr float PULSE_GROW_DURATION = 2.0f;
inline static constexpr float PULSE_POP_DURATION = 0.08f;
inline static constexpr float PULSE_SHRINK_DURATION = 0.3f;

enum class PulsePhase
{
    growing,
    popping,
    shrinking
};

struct PulseState
{
    float radius;
    float flash; //< 0 = SWEEP_COLOR, 1 = POP_FLASH_COLOR
};

inline constexpr PulseState operator+(const PulseState& a, const PulseState& b)
{
    return {a.radius + b.radius, a.flash + b.flash};
}

inline constexpr PulseState operator-(const PulseState& a, const PulseState& b)
{
    return {a.radius - b.radius, a.flash - b.flash};
}

inline constexpr PulseState operator*(const PulseState& a, float t)
{
    return {a.radius * t, a.flash * t};
}

inline static ValueTweenTransition<PulseState> pulse_growing()
{
    return valueTween(
        PulseState {PULSE_MIN_RADIUS, 0.0f},
        PulseState {PULSE_MAX_RADIUS, 0.0f},
        PULSE_GROW_DURATION,
        &curves::easeInOutSine
    );
}

inline static ValueTweenTransition<PulseState> pulse_popping()
{
    return valueTween(
        PulseState {PULSE_MAX_RADIUS, 0.0f},
        PulseState {PULSE_POP_RADIUS, 1.0f},
        PULSE_POP_DURATION,
        &curves::easeOutExpo
    );
}

inline static ValueTweenTransition<PulseState> pulse_shrinking()
{
    return valueTween(
        PulseState {PULSE_POP_RADIUS, 1.0f},
        PulseState {PULSE_MIN_RADIUS, 0.0f},
        PULSE_SHRINK_DURATION,
        &curves::easeOutQuad
    );
}

struct Echo
{
    float2 position;
    std::vector<Sweep> sweeps;
    ValueTweenTransition<PulseState> pulse;
    PulsePhase pulsePhase;
};

inline static void echo_update(Echo& echo, float deltaTime)
{
    echo.pulse.advance(deltaTime);
    if (echo.pulse.isFinished()) {
        switch (echo.pulsePhase) {
            case PulsePhase::growing:
                echo.sweeps.push_back(Sweep {
                    .radius = valueTween(0.0f, 200.0f, 2.0f, &curves::easeLinear),
                    .emitterPosition = echo.position,
                });
                echo.pulsePhase = PulsePhase::popping;
                echo.pulse = pulse_popping();
                break;
            case PulsePhase::popping:
                echo.pulsePhase = PulsePhase::shrinking;
                echo.pulse = pulse_shrinking();
                break;
            case PulsePhase::shrinking:
                echo.pulsePhase = PulsePhase::growing;
                echo.pulse = pulse_growing();
                break;
        }
    }

    for (size_t i = echo.sweeps.size(); i-- > 0;) {
        Sweep& sweep = echo.sweeps[i];
        sweep_update(sweep, deltaTime);

        if (sweep_done(sweep)) {
            echo.sweeps.erase(echo.sweeps.begin() + i);
        }
    }
}

inline static void echo_draw(const Echo& echo)
{
    const PulseState pulseState = echo.pulse.value();

    noStroke();
    fill(lerpColor(SWEEP_COLOR, POP_FLASH_COLOR, pulseState.flash));
    circle(echo.position.x, echo.position.y, pulseState.radius);

    for (const Sweep& sweep : echo.sweeps) {
        sweep_draw(sweep);
    }
}

struct EchoLocate : Sketch
{
    std::vector<LineSegment> segments;
    std::vector<Echo> echos;

    void setup() override
    {
        setWindowSize(800, 600);

        constexpr float windowMargin = 50.0f;

        for (size_t i = 0; i < 10; ++i) {
            constexpr float maxSegmentLength = 100.0f;
            constexpr float minSegmentLength = 20.0f;
            const float segmentLength = random(minSegmentLength, maxSegmentLength);
            const float segmentStartX = random(windowMargin, getWidth() - windowMargin);
            const float segmentStartY = random(windowMargin, getHeight() - windowMargin);
            const float segmentAngle = random(0.0f, TAU);
            const float segmentEndX = segmentStartX + segmentLength * cos(segmentAngle);
            const float segmentEndY = segmentStartY + segmentLength * sin(segmentAngle);

            segments.push_back(LineSegment {
                .start = float2 {.x = segmentStartX, .y = segmentStartY},
                .end = float2 {.x = segmentEndX, .y = segmentEndY},
                .flash = valueTween(0.0f, 0.0f, FLASH_DURATION, &curves::easeOutExpo),
            });
        }
    }

    void event(const WindowEvent& event) override
    {
        event.on(
            [this](const WindowEvent::KeyPress& event) {
                const float2 mousePos {.x = static_cast<float>(getMouseX()), .y = static_cast<float>(getMouseY())};

                if (event.key == Key::Space) {
                    echos.push_back(Echo {
                        .position = mousePos,
                        .pulse = pulse_growing(),
                        .pulsePhase = PulsePhase::growing,
                    });
                }
            }
        );
    }

    void draw() override
    {
        const float deltaTime = getDeltaTime();

        for (Echo& echo : echos) {
            echo_update(echo, deltaTime);
        }

        for (Echo& echo : echos) {
            for (LineSegment& segment : segments) {
                for (Sweep& sweep : echo.sweeps) {
                    if (sweep_reaches_segment(sweep, segment)) {
                        segment_flash(segment);
                    }
                }
            }
        }

        for (LineSegment& segment : segments) {
            segment_update(segment, deltaTime);
        }

        background(rgba(31, 31, 51));

        for (const LineSegment& segment : segments) {
            segment_draw(segment);
        }

        for (const Echo& echo : echos) {
            echo_draw(echo);
        }
    }
};

SketchSpec p5::createSpec()
{
    return SketchSpec {
        .sketch = [] {
            return std::make_unique<EchoLocate>();
        },
    };
}
