#include <p5cpp_webcam/p5cpp_webcam.hpp>
#include <p5cpp_webcam/capture_resource.hpp>

#include <ccap_c.h>

namespace p5::webcam
{
    class WebcamPlugin;
} // namespace p5::webcam

namespace p5::webcam
{
    inline static thread_local WebcamPlugin* activePlugin = nullptr;
} // namespace p5::webcam

namespace p5::webcam
{
    class WebcamPlugin : public Plugin
    {
    public:
        void setup([[maybe_unused]] Context& context, const Next& next) override
        {
            activePlugin = this;

            if (not registerErrorCallback()) {
                error("Failed to register error callback for CameraCapture library.");
            }

            m_provider = ccap_provider_create();
            if (m_provider == nullptr) {
                error("Failed to create CameraCapture provider.");
                next();
                return;
            }

            m_availableCameras = queryAvailableCameras(m_provider);
            if (m_availableCameras.empty()) {
                error("No available cameras found.");
            }

            next();
        }

        void destroy([[maybe_unused]] Context& context, const Next& next) override
        {
            next();

            if (m_provider != nullptr) {
                ccap_provider_destroy(m_provider);
                m_provider = nullptr;
            }

            activePlugin = nullptr;
        }

        std::span<const Webcam> getAvailableCameras() const
        {
            return m_availableCameras;
        }

    private:
        static bool registerErrorCallback()
        {
            return ccap_set_error_callback(
                [](CcapErrorCode errorCode, const char* errorDescription, [[maybe_unused]] void* userData) {
                    error("CameraCapture error (code {}): {}", static_cast<std::underlying_type_t<CcapErrorCode>>(errorCode), errorDescription);
                },
                nullptr
            );
        }

        static std::vector<Webcam> queryAvailableCameras(CcapProvider* provider)
        {
            CcapDeviceNamesList deviceList;
            if (not ccap_provider_find_device_names_list(provider, &deviceList)) {
                error("Failed to find device names list for CameraCapture library.");
                return {};
            }

            std::vector<Webcam> cameras;
            cameras.reserve(deviceList.deviceCount);

            for (size_t i = 0; i < deviceList.deviceCount; ++i) {
                cameras.push_back({.name = std::string(deviceList.deviceNames[i]), .globalIndex = i});
            }

            return cameras;
        }

        CcapProvider* m_provider = nullptr;
        std::vector<Webcam> m_availableCameras;
    };
} // namespace p5::webcam

namespace p5::webcam
{
    bool Capture::isValid() const
    {
        return resource != nullptr;
    }

    bool Capture::isFrameNew() const
    {
        if (resource == nullptr) {
            return false;
        }

        return resource->isFrameNew();
    }

    Texture Capture::getTexture() const
    {
        if (resource == nullptr) {
            return {};
        }

        return resource->getTexture();
    }

    Pixels Capture::getPixels() const
    {
        if (resource == nullptr) {
            return {};
        }

        return resource->getPixels();
    }

    float Capture::getFPS() const
    {
        if (resource == nullptr) {
            return 0.0f;
        }

        return resource->getFPS();
    }

    std::span<const WebcamResolution> Capture::getSupportedResolutions() const
    {
        if (resource == nullptr) {
            return {};
        }

        return resource->getSupportedResolutions();
    }

    void Capture::close()
    {
        if (resource != nullptr) {
            resource->close();
        }
    }
} // namespace p5::webcam

namespace p5::webcam
{
    std::span<const Webcam> getAvailableCameras()
    {
        if (activePlugin == nullptr) {
            error("Camera is not initialized. Please add the WebcamPlugin to your sketch.");
            return {};
        }

        return activePlugin->getAvailableCameras();
    }

    std::optional<Capture> openCapture(size_t deviceIndex, const CaptureOptions& options)
    {
        std::shared_ptr<CaptureResource> resource = CaptureResource::create(deviceIndex, options);
        if (resource == nullptr) {
            return std::nullopt;
        }

        return Capture {.resource = std::move(resource)};
    }
} // namespace p5::webcam

namespace p5::webcam
{
    std::unique_ptr<Plugin> createWebcamPlugin()
    {
        return std::make_unique<WebcamPlugin>();
    }
} // namespace p5::webcam
