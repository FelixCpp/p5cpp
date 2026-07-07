# AGENTS.md

Guidance for AI coding agents (and humans) working on **p5cpp**, a native C++23 creative
coding framework inspired by Processing / p5.js. Read this before making changes.

> **See also `SKILLS.md`** for concrete, step-by-step recipes (adding an API function,
> a new module, a new example, changing render state, …). Consult it whenever a task
> matches one of its recipes, and follow the file lists/order given there.

## 1. What this project is

p5cpp gives C++ users a p5.js-style global API (`background()`, `fill()`, `circle()`,
`translate()`, `loadFont()`, …) backed by OpenGL, GLFW, FreeType and libtess2. Users
subclass `p5cpp::Sketch` and implement `setup()` / `draw()`; the framework owns the
window, the render loop, and input dispatch. See `README.md` for the full user-facing
API and example sketches — treat it as the source of truth for public behaviour and
keep it in sync with any public API changes.

## 2. Build & verify

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --config Release --parallel
```

- Requires the git submodules under `external/` (glfw, freetype, glad, libtess2,
  miniaudio) — checked out with `--recursive` / `git submodule update --init --recursive`.
- C++ standard is C++23 (`CMAKE_CXX_STANDARD 23`), set in the top-level `CMakeLists.txt`.
- There is **no unit test suite**. CI (`.github/workflows/ci.yaml`) builds on Windows and
  macOS and verifies that every example binary under `examples/` was produced. **Building
  the library and all examples is the de-facto test** — always do this after a change,
  and prefer building on both fast-fail configs if unsure.
- New `.cpp` files must be added explicitly to the `p5cpp` (or `libtess2`) target's source
  list in the top-level `CMakeLists.txt` — there is no globbing.
- Adding a new example requires its own `CMakeLists.txt` under `examples/<name>/`, being
  added via `add_subdirectory` in `examples/CMakeLists.txt`, and being added to the binary
  list in `.github/workflows/ci.yaml` if it should be part of CI verification.

## 3. Architecture

### 3.1 Public vs internal split

- `include/p5cpp/` — the **public** API surface. Only headers that end users of the
  library should see. `include/p5cpp/p5cpp.hpp` is the single umbrella header re-exporting
  free functions (`setWindowSize`, `getMouseX`, drawing calls, …) and public types
  (`Sketch`, `Color`, `Font`, `Shader`, math types, …).
- `src/p5cpp/` — **internal** implementation, including private headers (e.g.
  `graphics_module.hpp`, `renderer.hpp`, `tess.hpp`). Never expose internal headers from
  `include/`. If a new capability needs new public surface, add a small header in
  `include/p5cpp/<domain>/` and forward-declare/implement details in `src/p5cpp/<domain>/`.
- Domains mirror each other in both trees: `application/`, `graphics/`, `math/`, `system/`.

### 3.2 Module / Component / API layering

Each subsystem (frame timing, window, input, graphics, sketch) follows the same
three-part pattern — copy it for any new subsystem:

1. **Component** (`*_component.hpp/.cpp`) — plain-old class holding state and behaviour
   for the subsystem (e.g. `FrameComponent`, `GraphicsComponent`). No knowledge of the
   module system; just a regular object.
2. **Module** (`*_module.hpp/.cpp`) — implements `p5cpp::Module` (see
   `include/p5cpp/application/module.hpp`), owns/creates the Component, registers it on
   the `AppContext` via `registerService<T>()`, and wires it into the engine's
   `setup` / `event` / `draw` / `destroy` lifecycle hooks.
3. **API** (`*_api.cpp`) — free functions that make up the public global API (what users
   call from `draw()`). Each looks up its component via
   `engine->getContext().require<XComponent>()` and forwards the call. This is how the
   global-function style of p5.js is achieved without global mutable state scattered
   everywhere — all state lives in components, reached indirectly through `AppContext`.

### 3.3 Engine as an onion / middleware pipeline

`Engine` (`src/p5cpp/application/engine.hpp/.cpp`) holds an ordered
`std::vector<std::unique_ptr<Module>>`. Each lifecycle call (`setup`, `event`, `draw`,
`destroy`) recursively invokes `modules[i]->fn(context, next)` where `next` is a
`std::function<void()>` continuation calling `modules[i+1]`. This is an Express.js-style
middleware chain: a module can do work, call `next()` to let downstream modules run, then
do more work afterwards (e.g. wrap the frame). **Module registration order in
`src/p5cpp/p5cpp.cpp` matters** — it currently is Frame → Window → Input → Graphics →
Sketch. Respect this order (or have a specific reason to change it) since later modules
depend on services registered by earlier ones (e.g. `SketchModule` expects window/graphics
services to already exist).

### 3.4 AppContext service locator

`AppContext` (`include/p5cpp/application/app_context.hpp`) is a tiny type-indexed service
locator: `registerService<T>(T*)`, `getOrNull<T>()`, `require<T>()`. Components register
themselves as services; API free-function files fetch them with `require<T>()`. When
adding a new component, register it in the owning module's `setup()` and unregister in
`destroy()` if it manages a resource with non-static lifetime.

### 3.5 Vendored dependencies

`external/` contains FreeType, GLAD, GLFW, libtess2 and miniaudio as git submodules,
built directly by the top-level `CMakeLists.txt`. Do not hand-edit vendored sources —
if a fix is needed there, update the submodule/pin instead.

## 4. Coding conventions

Formatting is enforced by `.clang-format` (run `clang-format` before committing changes
to `.hpp`/`.cpp` files). Key style points observed throughout the codebase — follow them
exactly, since they diverge from common defaults:

- 4-space indentation, **no column limit** (don't wrap lines artificially).
- Braces on their own line after `namespace`, `struct`, `class`, `function` definitions
  (`BreakBeforeBraces: Custom` / `AfterNamespace/AfterStruct/AfterClass/AfterFunction`),
  but `if`/`for`/`while` bodies keep the opening brace on the same line.
- Every namespace is closed with a matching `// namespace p5cpp` (or nested name) comment.
- It's idiomatic here to open a **new, separate** `namespace p5cpp { ... }` block for each
  logical group within one file (e.g. one block for a `class`, another for its factory
  function) — see `engine.cpp` and `frame_api.cpp`. Prefer this over one large block when
  mixing declarations and free functions.
- Private members are prefixed `m_` and camelCase (`m_frameComponent`, `m_component`).
- Types: `PascalCase` (`FrameComponent`, `GraphicsModule`, `WindowEvent`). Free functions
  and methods: `camelCase` (`getFrameCount`, `pushMatrix`). Type aliases for math vectors
  use `snake_case` with suffix (`float2`, `int2`, `color_t`, `matrix4x4`).
- Public API free functions in `include/p5cpp/p5cpp.hpp` mirror p5.js naming exactly
  where an equivalent exists (`background`, `fill`, `stroke`, `translate`, `noLoop`) —
  preserve this naming parity when extending the API instead of inventing new verbs.
- Prefer `explicit` on single-argument constructors (see `GraphicsComponent`).
- Header guards use `#pragma once` exclusively (no include guards).

## 5. Making changes safely

- When adding a new drawable/state feature (e.g. a new shape or render state field), the
  usual multi-file path is: `GraphicsComponent` (state + logic) → `GraphicsModule` (wiring,
  usually unchanged) → `graphics_api.cpp` (new free function) →
  `include/p5cpp/graphics/*.hpp` and `include/p5cpp/p5cpp.hpp` (public declaration) →
  update `README.md`'s API reference / examples if user-facing.
- When touching rendering internals (renderer, stroker, tess, matrix/render state stacks),
  check `render_state.hpp`/`render_state_stack.hpp` first — most draw state (fill, stroke,
  blend mode, matrix) is stack-based via `pushState`/`popState`, mirrored by
  `pushMatrix`/`popMatrix` for transforms only. Keep this separation.
- Since there's no test suite, validate changes by building the library **and** the
  `examples/` (they exercise most of the public API in realistic sketches) — a failed or
  crashing example is effectively a regression test failure.
- Don't modify files under `external/` — those are third-party vendored submodules.
