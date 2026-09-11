#include "ccap_utils_c.h"
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
        explicit WebcamPlugin(LogLevel level)
            : m_logLevel(level)
        {
        }

        void setup(const Next& next) override
        {
            activePlugin = this;

            if (not registerErrorCallback(m_logLevel)) {
                error("Failed to register error callback for CameraCapture library.");
            }

            next();
        }

        void destroy(const Next& next) override
        {
            next();

            activePlugin = nullptr;
        }

        std::span<const Webcam> getAvailableCameras()
        {
            if (m_availableCameras.empty()) {
                CcapProvider* provider = ccap_provider_create();
                if (provider == nullptr) {
                    error("Failed to create CameraCapture provider.");
                    return {};
                }

                m_availableCameras = queryAvailableCameras(provider);
                if (m_availableCameras.empty()) {
                    warn("No available cameras found.");
                }

                ccap_provider_destroy(provider);
            }

            return m_availableCameras;
        }

    private:
        static bool registerErrorCallback(const LogLevel level)
        {
            const CcapLogLevel ccapLogLevel = std::invoke([&]() {
                switch (level) {
                    case LogLevel::none: return CCAP_LOG_LEVEL_NONE;
                    case LogLevel::error: return CCAP_LOG_LEVEL_ERROR;
                    case LogLevel::warning: return CCAP_LOG_LEVEL_WARNING;
                    case LogLevel::info: return CCAP_LOG_LEVEL_INFO;
                    default: return CCAP_LOG_LEVEL_NONE;
                }
            });

            ccap_set_log_level(ccapLogLevel);

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

            std::vector<Webcam> webcams;
            webcams.reserve(deviceList.deviceCount);

            for (size_t i = 0; i < deviceList.deviceCount; ++i) {
                webcams.push_back({
                    .name = std::string(deviceList.deviceNames[i]),
                    .globalIndex = i,
                });
            }

            return webcams;
        }

        std::vector<Webcam> m_availableCameras;
        LogLevel m_logLevel;
    };
} // namespace p5::webcam

namespace p5::webcam
{
    bool Capture::update()
    {
        return resource->update();
    }

    std::optional<ReadOnlyPixels> Capture::loadPixels()
    {
        return resource->loadPixels();
    }

    std::optional<Texture> Capture::loadTexture()
    {
        return resource->loadTexture();
    }

    std::span<const WebcamResolution> Capture::getSupportedResolutions() const
    {
        return resource->getSupportedResolutions();
    }

    void Capture::close()
    {
        resource->close();
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
    std::unique_ptr<Plugin> createWebcamPlugin(LogLevel level)
    {
        return std::make_unique<WebcamPlugin>(level);
    }
} // namespace p5::webcam
