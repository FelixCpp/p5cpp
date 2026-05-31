# p5

A C++ creative-coding library inspired by [Processing](https://processing.org/) and [p5.js](https://p5js.org/). It provides a sketch-based programming model, a 2D drawing API, input handling, and GPU-accelerated rendering via OpenGL — all in a single namespace.

---

## Table of Contents

- [Getting Started](#getting-started)
- [Architecture](#architecture)
- [Features](#features)
  - [Sketch Lifecycle](#sketch-lifecycle)
  - [Drawing Primitives](#drawing-primitives)
  - [Custom Shapes](#custom-shapes)
  - [Fill, Stroke & Color](#fill-stroke--color)
  - [Transformations](#transformations)
  - [State Management](#state-management)
  - [Textures & Images](#textures--images)
  - [Off-Screen Rendering (Canvases)](#off-screen-rendering-canvases)
  - [Custom Shaders](#custom-shaders)
  - [Text & Fonts](#text--fonts)
  - [Math & Noise](#math--noise)
  - [Input & Events](#input--events)
  - [Window Management](#window-management)
  - [Timing & Frame Rate](#timing--frame-rate)
  - [Logging](#logging)
- [Building](#building)
- [Dependencies](#dependencies)
- [Limitations & Missing Features](#limitations--missing-features)

---

## Getting Started

Implement a sketch by deriving from `p5::Sketch` and providing a factory function:

```cpp
#include "p5.hpp"

struct MySketch : p5::Sketch
{
    void setup() override
    {
        p5::setWindowSize(800, 600);
        p5::setWindowTitle("Hello p5");
    }

    void draw() override
    {
        p5::background(30);
        p5::fill(255, 100, 50);
        p5::noStroke();
        p5::circle(p5::width / 2.0f, p5::height / 2.0f, 100.0f);
    }

    void destroy() override {}
};

std::unique_ptr<p5::Sketch> p5::createSketch()
{
    return std::make_unique<MySketch>();
}
```

---

## Architecture

| Layer | Files | Responsibility |
|---|---|---|
| **Public API** | `p5.hpp` / `p5.cpp` | Everything a sketch author touches |
| **Render state** | `renderstate.*` | Per-frame draw settings (color, stroke, blend, …) |
| **Matrix stack** | `matrixstack.*` | Hierarchical 2D transform stack |
| **Canvas stack** | `canvas.*` | Push/pop render targets (FBOs) |
| **Draw buffer** | `drawscope.*` | CPU-side vertex/index buffer filled each frame |
| **Stroker** | `stroker.*` | Analytical stroke tessellation (caps, joins, dashes) |
| **Tessellator** | `tess.*` | Polygon fill tessellation via libtess2 |
| **Line path** | `linepath.*` | Vertex accumulator for shape primitives |
| **Renderer** | `rendering.*` | OpenGL draw calls, VAO/VBO management |
| **Window** | `window.*` | GLFW window, HiDPI framebuffer, event dispatch |
| **Shader** | `shader.*` | GLSL program compilation & uniform cache |
| **Font** | `font.cpp` | FreeType-backed glyph atlas |
| **Framebuffer** | `framebuffer.*` | Off-screen FBO wrapper |
| **Texture** | `texture.cpp` | Texture loading & upload |
| **Timing** | `timing.*` | Frame-rate limiter, delta time, millis |
| **Noise** | `noise.cpp` | Simplex noise (1-D / 2-D / 3-D) |
| **Math** | `math.cpp` | Random, remap, trig helpers |

---

## Features

### Sketch Lifecycle

```cpp
struct Sketch {
    virtual void setup()   = 0;   // called once before the first frame
    virtual void draw()    = 0;   // called every frame
    virtual void destroy() = 0;   // called on shutdown
    virtual void event(const WindowEvent&) {}  // optional input handler
};
```

### Drawing Primitives

```cpp
p5::point(x, y);
p5::line(x1, y1, x2, y2);
p5::rect(left, top, width, height);
p5::rect(left, top, width, height, cornerRadiusX, cornerRadiusY);   // uniform corner radius
p5::rect(left, top, w, h, tlX, tlY, trX, trY, brX, brY, blX, blY); // per-corner radius
p5::square(left, top, size);
p5::ellipse(cx, cy, width, height);
p5::circle(cx, cy, diameter);
p5::triangle(x1, y1, x2, y2, x3, y3);
p5::arc(cx, cy, width, height, startAngle, sweepAngle, ArcMode::open | chord | pie);
p5::bezier(x1, y1, x2, y2, x3, y3, x4, y4);   // cubic Bézier
p5::curve(x1, y1, x2, y2, x3, y3, x4, y4);    // Catmull-Rom spline
```

### Custom Shapes

```cpp
p5::beginShape();
p5::vertex(x, y);              // position only
p5::vertex(x, y, u, v);        // with texture coordinates
p5::curveVertex(x, y);         // Catmull-Rom point
p5::endShape(ShapeType::polygon, /*close=*/true);
```

Supported shape types: `lines`, `lineStrip`, `lineLoop`, `triangles`, `triangleStrip`, `triangleFan`, `quads`, `quadStrip`, `polygon`.

### Fill, Stroke & Color

```cpp
// Colors
p5::color_t c = p5::color(r, g, b, a);
p5::color_t grey = p5::color(128);
p5::color_t mid = p5::lerp(c1, c2, 0.5f);
int r = p5::red(c);  // also green(), blue(), alpha(), brightness()

// Fill
p5::fill(r, g, b, a);
p5::noFill();

// Stroke
p5::stroke(r, g, b, a);
p5::noStroke();
p5::strokeWeight(2.0f);
p5::strokeCap(StrokeCap::round);    // butt | square | round
p5::strokeJoin(StrokeJoin::miter);  // miter | bevel | round
p5::miterLimit(10.0f);
p5::roundJoinThreshold(0.1f);

// Dashed strokes
float pattern[] = {10.0f, 5.0f};
p5::strokePattern(pattern);
p5::strokePatternOffset(0.0f);

// Blend mode
p5::blendMode(BlendMode::alpha);    // none | alpha | additive | multiply

// Curve detail
p5::bezierDetail(20);
p5::curveTightness(0.0f);
p5::curveDetail(20);
```

### Transformations

```cpp
p5::translate(x, y);
p5::scale(sx, sy);
p5::rotate(angleInRadians);

p5::pushMatrix();
// … nested transforms …
p5::popMatrix();

p5::resetMatrix();
p5::applyMatrix(mat);
p5::setMatrix(mat);
p5::matrix4x4& m = p5::peekMatrix();
```

Matrix helpers: `translation`, `scaling`, `rotation`, `ortho`, `perspective`, `lookAt`, `combine`, `transformPoint`.

### State Management

`pushState()` / `popState()` snapshot the entire render state (fill, stroke, blend mode, transforms, font, shader, …) and restore it later — useful for composing reusable drawing helpers.

### Textures & Images

```cpp
auto tex = p5::loadTexture("path/to/image.png");
auto tex2 = p5::loadTexture(width, height, pixelData);

p5::tint(r, g, b, a);   // colorize subsequent image draws
p5::noTint();
p5::image(tex->getRendererId(), left, top, width, height);
```

### Off-Screen Rendering (Canvases)

```cpp
auto canvas = p5::createCanvas(512, 512);

p5::pushCanvas(canvas);
// everything drawn here goes into the off-screen FBO
p5::background(0);
p5::circle(256, 256, 200);
p5::popCanvas();

// use the canvas as a texture
p5::image(canvas->getTextureId(), 0, 0, 512, 512);
```

### Custom Shaders

```cpp
auto sh = p5::loadShader(vertexSrc, fragmentSrc);
p5::shader(sh);
p5::setUniform("uTime",  p5::uniform(p5::millis() / 1000.0f));
p5::setUniform("uColor", p5::uniform(1.0f, 0.5f, 0.0f, 1.0f));
// draw calls …
p5::noShader();  // restore default shader
```

Uniform types: `float`, `float2`, `float4`, `matrix4x4`.

### Text & Fonts

```cpp
auto font = p5::loadFont("path/to/font.ttf");
// or from memory:
auto font2 = p5::loadFont(fontBytes);

p5::textFont(font);
p5::textSize(24.0f);
p5::textAlign(HorizontalTextAlign::center, VerticalTextAlign::baseline);
p5::fill(255);
p5::text("Hello!", x, y);

// Measurement
p5::TextMetrics m = p5::measureText("Hello!");
// m.width, m.totalHeight, m.ascender, m.descender, m.lineCount
```

A bundled copy of DejaVu Sans (`dejavusans.hpp`) is available for use without any external font file.

### Math & Noise

```cpp
// Helpers
float r  = p5::radians(180.0f);
float d  = p5::degrees(r);
float v  = p5::remap(value, fromLow, fromHigh, toLow, toHigh);

// Random
p5::randomSeed(42);
float x = p5::random(100.0f);
float y = p5::random(-1.0f, 1.0f);

// Simplex noise (returns values in roughly [-1, 1])
float n1 = p5::noise(x);
float n2 = p5::noise(x, y);
float n3 = p5::noise(x, y, z);

// Fractional Brownian Motion
p5::SimplexNoise fbm(/*frequency*/1.0f, /*amplitude*/1.0f,
                     /*lacunarity*/2.0f, /*persistence*/0.5f);
float f = fbm.fractal(6, x, y);
```

2-D vector type `float2` supports `+`, `-`, `*`, `/`, `dot`, `cross`, `lerp`, `length`, `lengthSquared`, `normalized`, `perp`.

### Input & Events

Override `event(const WindowEvent&)` in your sketch:

```cpp
void event(const p5::WindowEvent& e) override
{
    switch (e.type) {
        case p5::EventType::mousePress:
            p5::info("click at " + std::to_string(e.mouseButton.x));
            break;
        case p5::EventType::keyPress:
            if (e.keyEvent.key == p5::Key::space) { /* … */ }
            break;
        default: break;
    }
}
```

| Event type | Data field | Contents |
|---|---|---|
| `mouseMove` | `mouseMove` | `x`, `y` |
| `mousePress` / `mouseRelease` | `mouseButton` | `button`, `x`, `y` |
| `mouseScroll` | `mouseScroll` | `dx`, `dy`, `x`, `y` |
| `keyPress` / `keyRelease` / `keyRepeat` | `keyEvent` | `key`, `mods` |
| `character` | `charEvent` | `codepoint` (UTF-32) |
| `windowResize` | `windowResize` | `width`, `height` |
| `close` | — | window close request |

Current mouse position is always available via `p5::mouseX` / `p5::mouseY`.

Modifier bitmasks: `KeyMod::shift`, `KeyMod::ctrl`, `KeyMod::alt`, `KeyMod::super`.

### Window Management

```cpp
p5::setWindowSize(1280, 720);
p5::setWindowTitle("My Sketch");
p5::setWindowResizable(true);
int w = p5::getWindowWidth();
int h = p5::getWindowHeight();
```

HiDPI / Retina displays are handled transparently: logical coordinates match `p5::width` / `p5::height`, while the internal viewport uses physical pixel dimensions.

### Timing & Frame Rate

```cpp
p5::frameRate(60.0f);   // cap to 60 fps; 0 = unlimited
p5::noLoop();           // pause the draw loop
p5::loop();             // resume
bool running = p5::isLooping();

float ms = p5::millis();    // milliseconds since start
float dt = p5::deltaTime;   // seconds since last frame
float f  = p5::fps;         // smoothed FPS
int   fc = p5::frameCount;  // frames drawn so far
```

### Logging

```cpp
p5::info("loaded texture");
p5::debug("vertex count: 1024");
p5::warning("slow frame detected");
p5::error("failed to open file");
```

---

## Building

```bash
git clone --recurse-submodules <repo-url>
cd p5
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -Wno-dev
cmake --build . --target app -j$(sysctl -n hw.logicalcpu)
```

Replace `app` with another target name as needed.

**Requirements:** CMake ≥ 3.20, a C++23-capable compiler (Clang / GCC / MSVC).

---

## Dependencies

All dependencies are included as Git submodules under `external/`:

| Library | Purpose |
|---|---|
| [GLFW](https://www.glfw.org/) | Window creation & input |
| [GLAD](https://glad.dav1d.de/) | OpenGL function loader |
| [FreeType](https://freetype.org/) | Font rasterization |
| [libtess2](https://github.com/memononen/libtess2) | Polygon tessellation |

---

## Limitations & Missing Features

The following features from Processing / p5.js are **not yet implemented**:

| Feature | Notes |
|---|---|
| **3D primitives** | `box()`, `sphere()`, `cylinder()` etc. — projection matrices exist but there is no 3D draw pipeline |
| **HSB / HSL color mode** | Only RGB is supported; `colorMode()` does not exist |
| **Pixel-level image access** | No `get(x, y)` / `set(x, y)` / `pixels[]`; images are opaque GPU textures |
| **Image filters & effects** | No blur, threshold, tint-on-image, or similar post-processing helpers |
| **Image saving / export** | No `save()`, `saveFrame()`, PDF or SVG export |
| **`loadImage` / `PImage`** | Textures are loaded via `loadTexture()`; there is no higher-level image class |
| **Audio / sound** | No audio API |
| **Video** | No video playback or capture |
| **Touch input** | No touch events |
| **Network / HTTP** | No `loadJSON()`, `loadStrings()`, or HTTP helpers |
| **`constrain()`** | No built-in clamping helper (use `std::clamp`) |
| **`dist()` / `mag()`** | Distance and magnitude helpers are not in the public API |
| **`map()`** | Available as `p5::remap()` |
| **`print()` / `println()`** | Use `p5::info()` / `p5::debug()` instead |
| **Easing / tweening** | No built-in animation curves |
| **Multiple windows** | Single-window only |
