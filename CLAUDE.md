# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

p5cpp is a native C++23 creative-coding framework that mirrors the p5.js/Processing API (`background()`, `fill()`, `circle()`, `noise()`, shaders, framebuffers) on top of OpenGL/GLFW. Users write a `Sketch` subclass with `setup()`/`draw()`/`event()` and the framework owns the window and render loop.

## Build

All dependencies (GLFW, FreeType, GLAD, libtess2, harfbuzz, miniaudio) are bundled as git submodules — clone with `--recursive` or run `git submodule update --init --recursive` if they're missing.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

There are no unit tests in this repo. Verification is done by building the example programs — CI (`.github/workflows/ci.yaml`) configures/builds on Windows and macOS and then checks that every example binary under `build/examples/<name>/<name>` (or `build/examples/<name>/Release/<name>.exe` on Windows) exists. When adding a new example, add its subdirectory to `examples/CMakeLists.txt` and also add its name to both the Linux/macOS and Windows binary-verification lists in the CI workflow.

To build/run a single example during development:

```bash
cmake --build build --target demo --parallel
./build/examples/demo/demo
```

## Architecture

### Middleware pipeline (`Module`)

The engine (`src/p5cpp/application/engine.cpp`, `AppEngine`) is a linear chain of `Module`s (`include/p5cpp/application/module.hpp`), run in registration order for `setup`, `draw`, `event`, and `destroy`. Each module receives a `Next next` callback it must invoke to pass control to the next module — this is an explicit middleware/onion pattern (like Express/Koa), not a fixed set of virtual hooks. A module can do work before _and_ after `next()` (e.g. `SketchModule::destroy` calls `next()` first, then tears down the sketch afterward). Module registration order matters and lives in `src/p5cpp/p5cpp.cpp`'s `main()`:

```
FrameModule → WindowModule → InputModule → GraphicsModule → SketchModule
```

Modules communicate through a shared `AppContext` (`include/p5cpp/application/app_context.hpp`), a simple type-indexed service locator (`registerService<T>` / `require<T>` / `getOrNull<T>`). There's no DI container beyond this.

### Module vs. Component vs. free-function API

Each subsystem (application, graphics, ...) is split into three layers:

- **Module** (`*_module.hpp/.cpp`) — the `Module` subclass wired into the engine pipeline; owns lifecycle and registers a Component as a service.
- **Component** (`*_component.hpp/.cpp`) — the actual owned state/logic for that subsystem (e.g. `GraphicsComponent` holds the matrix stack, render state stack, renderer).
- **API** (`*_api.cpp`) — the public free functions declared in `include/p5cpp/...` (e.g. `fill()`, `translate()`, `pushMatrix()`) that a sketch author calls directly. These look up the relevant Component via the global `engine->getContext().require<T>()` and forward the call. See `src/p5cpp/graphics/graphics_api.cpp` for the pattern — nearly every public drawing/state function is a one-line forward to `GraphicsComponent`.

When adding a new public API function: add the declaration to the appropriate header under `include/p5cpp/`, implement the logic on the relevant `*Component`, and add a one-line forwarding definition in the matching `*_api.cpp`.

### Resource loading

`loadFont()`/`loadShader()` (`src/p5cpp/graphics/font.cpp`, `shader.cpp`) cache by path/source: calling the same load function again with the same arguments while a previous handle from that call is still alive returns a new lightweight handle sharing the same underlying parsed font / compiled shader program, instead of reloading from disk or recompiling. This is transparent for these read-only resource types (no per-instance mutable state), but it does mean two `Font`/`Shader` values built from the same path/source are now aliases of one underlying resource. `loadImage()`/`loadTexture()` are deliberately **not** cached this way, since `Texture::upload()` is mutable and aliasing would make one texture's pixel upload silently visible through another.

### Sketch entry point

User code implements `p5cpp::Sketch` (`include/p5cpp/application/sketch.hpp`) and defines `p5cpp::createSketch()`, which `SketchModule` calls once during `setup`. `Sketch::registerModules(Engine&)` is the extension point for registering additional custom `Module`s before the sketch itself runs (see the "Custom Engine Modules" section of README.md for an example — e.g. a shared `TimerModule` other modules/sketches can `require<T>()` from `AppContext`).

### Directory layout

- `include/p5cpp/` — public API surface (what sketch authors `#include`).
- `src/p5cpp/` — implementation, organized by subsystem: `application/` (engine, window, input, sketch, frame timing), `graphics/` (renderer, shaders, text shaping/tessellation, framebuffers, matrix/render-state stacks), `math/` (vectors, matrices, noise, random), `system/` (timing, UTF-8 handling).
- `examples/` — one subdirectory per example sketch, each with its own `CMakeLists.txt` producing an executable linked against the `p5cpp` static library. `examples/CMakeLists.txt` lists all subdirectories to build.
- `external/` — bundled dependencies as git submodules; not modified directly.
