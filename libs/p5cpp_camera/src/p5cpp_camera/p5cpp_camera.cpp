#include <p5cpp_camera/p5cpp_camera.hpp>

#include <algorithm>
#include <cmath>

namespace p5::camera
{
    class CameraController
    {
    public:
        void update()
        {
            if (m_panEnabled and isMouseButtonDown(MouseButton::Middle)) {
                m_pan.x += static_cast<float>(getMouseDeltaX());
                m_pan.y += static_cast<float>(getMouseDeltaY());
            }

            const double scroll = getScrollY();
            if (m_zoomEnabled and scroll != 0.0) {
                const float factor = std::pow(1.0f + kZoomSensitivity, static_cast<float>(scroll));
                zoomAround(static_cast<float>(getMouseX()), static_cast<float>(getMouseY()), factor);
            }
        }

        void begin() const
        {
            pushMatrix();
            translate(m_pan.x, m_pan.y);
            scale(m_zoom, m_zoom);
        }

        void end() const
        {
            popMatrix();
        }

        float2 screenToWorld(float screenX, float screenY) const
        {
            return {(screenX - m_pan.x) / m_zoom, (screenY - m_pan.y) / m_zoom};
        }

        float2 worldToScreen(float worldX, float worldY) const
        {
            return {worldX * m_zoom + m_pan.x, worldY * m_zoom + m_pan.y};
        }

        float2 getPan() const
        {
            return m_pan;
        }

        float getZoom() const
        {
            return m_zoom;
        }

        void reset()
        {
            m_pan = {0.0f, 0.0f};
            m_zoom = 1.0f;
        }

        void setPan(float2 pan)
        {
            m_pan = pan;
        }

        void setZoom(float zoom)
        {
            m_zoom = std::clamp(zoom, m_minZoom, m_maxZoom);
        }

        void setZoomRange(float minZoom, float maxZoom)
        {
            m_minZoom = minZoom;
            m_maxZoom = maxZoom;
            m_zoom = std::clamp(m_zoom, m_minZoom, m_maxZoom); // don't leave zoom outside the new range
        }

        void setPanEnabled(bool enabled)
        {
            m_panEnabled = enabled;
        }

        bool isPanEnabled() const
        {
            return m_panEnabled;
        }

        void setZoomEnabled(bool enabled)
        {
            m_zoomEnabled = enabled;
        }

        bool isZoomEnabled() const
        {
            return m_zoomEnabled;
        }

    private:
        void zoomAround(float screenX, float screenY, float factor)
        {
            const float newZoom = std::clamp(m_zoom * factor, m_minZoom, m_maxZoom);
            const float appliedFactor = newZoom / m_zoom;

            m_pan.x = screenX - (screenX - m_pan.x) * appliedFactor;
            m_pan.y = screenY - (screenY - m_pan.y) * appliedFactor;
            m_zoom = newZoom;
        }

        static constexpr float kZoomSensitivity = 0.1f;

        float2 m_pan {0.0f, 0.0f};
        float m_zoom = 1.0f;
        float m_minZoom = 0.05f;
        float m_maxZoom = 40.0f;

        bool m_panEnabled = true;
        bool m_zoomEnabled = true;
    };
} // namespace p5::camera

namespace p5::camera
{
    inline static thread_local std::unique_ptr<CameraController> controller;
} // namespace p5::camera

namespace p5::camera
{
    class CameraPlugin : public Plugin
    {
    public:
        void setup(Context& context, const Next& next) override
        {
            controller = std::make_unique<CameraController>();
            context.provide(controller.get());

            next();
        }

        void draw([[maybe_unused]] Context& context, const Next& next) override
        {
            controller->update();
            next();
        }

        void destroy(Context& context, const Next& next) override
        {
            next();

            context.remove<CameraController>();
            controller.reset();
        }
    };
} // namespace p5::camera

namespace p5::camera
{
    void beginCamera()
    {
        if (controller == nullptr) {
            error("Camera is not initialized. Please add the CameraPlugin to your sketch.");
            return;
        }

        controller->begin();
    }

    void endCamera()
    {
        if (controller == nullptr) {
            error("Camera is not initialized. Please add the CameraPlugin to your sketch.");
            return;
        }

        controller->end();
    }

    float2 screenToWorld(float screenX, float screenY)
    {
        if (controller == nullptr) {
            error("Camera is not initialized. Please add the CameraPlugin to your sketch.");
            return {screenX, screenY};
        }

        return controller->screenToWorld(screenX, screenY);
    }

    float2 worldToScreen(float worldX, float worldY)
    {
        if (controller == nullptr) {
            error("Camera is not initialized. Please add the CameraPlugin to your sketch.");
            return {worldX, worldY};
        }

        return controller->worldToScreen(worldX, worldY);
    }

    float2 getCameraPan()
    {
        return controller != nullptr ? controller->getPan() : float2 {0.0f, 0.0f};
    }

    float getCameraZoom()
    {
        return controller != nullptr ? controller->getZoom() : 1.0f;
    }

    void resetCamera()
    {
        if (controller != nullptr) {
            controller->reset();
        }
    }

    void setCameraPan(float x, float y)
    {
        if (controller != nullptr) {
            controller->setPan({x, y});
        }
    }

    void setCameraZoom(float zoom)
    {
        if (controller != nullptr) {
            controller->setZoom(zoom);
        }
    }

    void setCameraZoomRange(float minZoom, float maxZoom)
    {
        if (controller != nullptr) {
            controller->setZoomRange(minZoom, maxZoom);
        }
    }

    void setCameraPanEnabled(bool enabled)
    {
        if (controller != nullptr) {
            controller->setPanEnabled(enabled);
        }
    }

    void setCameraZoomEnabled(bool enabled)
    {
        if (controller != nullptr) {
            controller->setZoomEnabled(enabled);
        }
    }

    bool isCameraPanEnabled()
    {
        return controller == nullptr or controller->isPanEnabled();
    }

    bool isCameraZoomEnabled()
    {
        return controller == nullptr or controller->isZoomEnabled();
    }
} // namespace p5::camera

namespace p5::camera
{
    std::unique_ptr<Plugin> createCameraPlugin()
    {
        return std::make_unique<CameraPlugin>();
    }
} // namespace p5::camera
