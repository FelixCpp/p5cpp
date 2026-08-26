#include "camera.hpp"

namespace kingshot
{
    using p5::float2;

    float2 worldToScreen(const Camera& camera, const float2& worldPos, const float2& screenCenter)
    {
        return (worldPos - camera.position) * camera.zoom + screenCenter;
    }

    float2 screenToWorld(const Camera& camera, const float2& screenPos, const float2& screenCenter)
    {
        return (screenPos - screenCenter) / camera.zoom + camera.position;
    }

    void panByScreenDelta(Camera& camera, const float2& screenDelta)
    {
        camera.position = camera.position - screenDelta / camera.zoom;
    }

    void zoomAtScreenPoint(Camera& camera, const float2& screenPos, const float2& screenCenter, float zoomFactor)
    {
        const float2 worldUnderCursor = screenToWorld(camera, screenPos, screenCenter);

        camera.zoom = p5::constrain(camera.zoom * zoomFactor, Camera::minZoom, Camera::maxZoom);

        // Re-derive camera.position so worldUnderCursor still projects to screenPos
        // under the new zoom -- inverse of screenToWorld's formula.
        camera.position = worldUnderCursor - (screenPos - screenCenter) / camera.zoom;
    }

    void applyCameraTransform(const Camera& camera, const float2& screenCenter)
    {
        p5::translate(screenCenter.x, screenCenter.y);
        p5::scale(camera.zoom, camera.zoom);
        p5::translate(-camera.position.x, -camera.position.y);
    }
} // namespace kingshot
