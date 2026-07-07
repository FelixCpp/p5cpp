# SKILLS.md

Task recipes for common p5cpp changes. Each recipe lists the concrete files to touch, in
order. Pair this with `AGENTS.md` for the underlying architecture/conventions before
using any recipe below.

## Add a new global API function (e.g. a new drawing primitive)

1. Decide which component owns the behaviour (usually `GraphicsComponent` for drawing,
   `FrameComponent` for timing, `InputComponent`/`WindowComponent` for input/window).
   Implement the logic there first (`src/p5cpp/<domain>/<domain>_component.{hpp,cpp}`).
2. Add a thin free function in the matching `*_api.cpp`
   (e.g. `src/p5cpp/graphics/graphics_api.cpp`) that resolves the component via
   `engine->getContext().require<XComponent>()` and forwards the call.
3. Declare the free function's public signature in the relevant public header under
   `include/p5cpp/<domain>/` and make sure it's reachable via
   `include/p5cpp/p5cpp.hpp` (add an include or declare inline if it lives directly
   in `p5cpp.hpp`, matching how neighbouring functions are declared there).
4. Update `README.md` — add/extend the relevant example snippet and the "API Reference
   Summary" table so user docs stay accurate.
5. Build the lib and all examples (see AGENTS.md §2). If an existing example would
   benefit from the new function, consider adding a short usage snippet, but don't
   invent a whole new example unless asked.

## Add a brand-new subsystem/module

1. Create `src/p5cpp/<domain>/<name>_component.{hpp,cpp}` — plain state/logic class,
   no dependency on `Module`/`AppContext`.
2. Create `src/p5cpp/<domain>/<name>_module.{hpp,cpp}` — subclass `p5cpp::Module`
   (`include/p5cpp/application/module.hpp`), own a `<Name>Component` (by value or
   `std::unique_ptr`), `registerService<XComponent>()` in `setup()`, and call
   `next()` in every overridden lifecycle method (`setup`/`event`/`draw`/`destroy`)
   after doing your own work — omitting `next()` breaks every module registered after
   yours.
3. Register the module in `src/p5cpp/p5cpp.cpp`'s `main()`, in the correct position in
   the pipeline (does it need services from Frame/Window/Input/Graphics? place it after
   those; does something else need your service? place it before that).
4. Add the new `.cpp` files to the `p5cpp` target's source list in the top-level
   `CMakeLists.txt` (no globbing — files not listed are silently not compiled).
5. Expose any public API as described in the recipe above.
6. Build and verify all examples still work.

## Add a new example

1. Create `examples/<name>/main.cpp` implementing `p5cpp::Sketch` (see any existing
   example, e.g. `examples/molds/main.cpp`, for the minimal shape) and
   `examples/<name>/CMakeLists.txt` (copy an existing one, adjust the target name).
2. Add `add_subdirectory(<name>)` to `examples/CMakeLists.txt`.
3. Add `<name>` to the binary-verification lists in both the Linux/macOS and Windows
   steps of `.github/workflows/ci.yaml` so CI actually checks it gets built.
4. Optionally add a screenshot to `gitassets/` and reference it in `README.md`'s
   screenshots section.
5. Build and confirm the new binary is produced under `build/examples/<name>/`.

## Modify render state (fill/stroke/blend mode/matrix)

1. State fields live in `src/p5cpp/graphics/render_state.hpp/.cpp`; the stack that
   supports `pushState()`/`popState()` is `render_state_stack.hpp/.cpp`. Transform
   matrices are a **separate** stack (`matrix_stack.hpp/.cpp`, `pushMatrix`/`popMatrix`)
   — don't conflate the two even though both are "state".
2. `GraphicsComponent` exposes the setters/getters used by `graphics_api.cpp`; add your
   new state field there, then thread it through `RenderState`/`RenderStateStack` as
   needed, then expose via `GraphicsComponent`, then via the API layer.
3. Rebuild and manually sanity-check with an example that exercises `pushState()`/
   `popState()` around your new state to confirm it saves/restores correctly.

## Verify a change (always do this, no unit tests exist)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --config Release --parallel
```

Confirm all binaries under `build/examples/*/` were rebuilt without errors. Run at least
one affected example manually if the change is visual/behavioural (window/graphics/input)
— headless builds do not exercise runtime GL/GLFW/FreeType behaviour.
