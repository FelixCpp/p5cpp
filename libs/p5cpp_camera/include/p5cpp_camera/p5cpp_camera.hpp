#pragma once

#include <p5cpp/p5cpp.hpp>

namespace p5::camera
{
    void beginCamera();
    void endCamera();

    template <typename Func>
        requires std::invocable<Func>
    void withCamera(Func&& func);

    float2 screenToWorld(float screenX, float screenY);
    float2 worldToScreen(float worldX, float worldY);

    float2 getCameraPan();
    float getCameraZoom();
    void resetCamera();

    void setCameraPan(float x, float y);
    void setCameraZoom(float zoom);

    void setCameraZoomRange(float minZoom, float maxZoom);

    void setCameraPanEnabled(bool enabled);
    void setCameraZoomEnabled(bool enabled);
    bool isCameraPanEnabled();
    bool isCameraZoomEnabled();
} // namespace p5::camera

namespace p5::camera
{
    std::unique_ptr<Plugin> createCameraPlugin();
} // namespace p5::camera

namespace p5::camera
{
    template <typename Func>
        requires std::invocable<Func>
    inline void withCamera(Func&& func)
    {
        try {
            beginCamera();
            func();
            endCamera();
        } catch (...) {
            endCamera();
            throw;
        }
    }
} // namespace p5::camera
