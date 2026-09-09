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
        uint32_t requestedWidth;
        uint32_t requestedHeight;
        float requestedFPS;
        bool flipHorizontal = false;
    };

    struct CaptureResource;
    struct Capture
    {
        std::shared_ptr<CaptureResource> resource;

        // Blocks (up to 500ms) grabbing the next frame from the camera. Call this once per draw()
        // before reading loadPixels()/loadTexture(); they never grab a frame themselves, so calling
        // either (or both) any number of times between two update() calls just re-reads the same
        // frame instead of hitting the camera again. Returns false if no frame could be grabbed.
        bool update();

        std::optional<ReadOnlyPixels> loadPixels();
        std::optional<Texture> loadTexture();
        std::span<const WebcamResolution> getSupportedResolutions() const;
        void close();
    };

    std::optional<Capture> openCapture(size_t deviceIndex = 0, const CaptureOptions& options = {});
} // namespace p5::webcam

namespace p5::webcam
{
    enum class LogLevel
    {
        none,
        error,
        warning,
        info,
    };

    std::unique_ptr<Plugin> createWebcamPlugin(LogLevel level = LogLevel::warning);
} // namespace p5::webcam
