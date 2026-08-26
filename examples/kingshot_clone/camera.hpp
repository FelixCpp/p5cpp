#pragma once

#include <p5cpp/p5cpp.hpp>

namespace kingshot
{
    // A simple 2D top-down RTS camera: `position` is the world-space point the
    // camera centers on, `zoom` scales world units to screen pixels. Screen-space
    // origin is assumed to be the center of the viewport (see worldToScreen).
    struct Camera
    {
        p5::float2 position {0.0f, 0.0f};
        float zoom = 1.0f;

        static constexpr float minZoom = 0.35f;
        static constexpr float maxZoom = 3.0f;
    };

    p5::float2 worldToScreen(const Camera& camera, const p5::float2& worldPos, const p5::float2& screenCenter);
    p5::float2 screenToWorld(const Camera& camera, const p5::float2& screenPos, const p5::float2& screenCenter);

    // Moves the camera by a screen-space drag delta (e.g. right-mouse drag), so the
    // world appears to follow the cursor like a grab-scroll.
    void panByScreenDelta(Camera& camera, const p5::float2& screenDelta);

    // Zooms while keeping the world point currently under `screenPos` fixed on screen,
    // so scrolling feels anchored to the cursor rather than to the viewport center.
    void zoomAtScreenPoint(Camera& camera, const p5::float2& screenPos, const p5::float2& screenCenter, float zoomFactor);

    // Applies the camera's translate+scale to the current p5 matrix stack. Caller is
    // expected to wrap this in withMatrix() so the transform doesn't leak.
    void applyCameraTransform(const Camera& camera, const p5::float2& screenCenter);
} // namespace kingshot
