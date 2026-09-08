#pragma once

#include <p5cpp/p5cpp.hpp>

namespace p5::webcam
{
    struct Webcam
    {
        std::string name;
        size_t globalIndex;
    };

    std::span<const Webcam> getAvailableCameras();
} // namespace p5::webcam

namespace p5::webcam
{
    struct WebcamResolution
    {
        uint32_t width;
        uint32_t height;
    };

    struct CaptureOptions
    {
        uint32_t requestedWidth = 0;
        uint32_t requestedHeight = 0;
        float requestedFPS = 0.0f;
    };

    struct CaptureResource;
    struct Capture
    {
        std::shared_ptr<CaptureResource> resource;

        bool operator==(const Capture&) const = default;
        bool isValid() const;
        bool isFrameNew() const;
        Texture getTexture() const;
        Pixels getPixels() const;
        float getFPS() const;
        std::span<const WebcamResolution> getSupportedResolutions() const;
        void close();
    };

    std::optional<Capture> openCapture(size_t deviceIndex = 0, const CaptureOptions& options = {});
} // namespace p5::webcam

namespace p5::webcam
{
    std::unique_ptr<Plugin> createWebcamPlugin();
} // namespace p5::webcam
