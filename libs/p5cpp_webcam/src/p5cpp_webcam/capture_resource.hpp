#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp_webcam/p5cpp_webcam.hpp>
#include <p5cpp_webcam/gpu_pixel_stream.hpp>

#include <ccap_c.h>

#include <vector>

namespace p5::webcam
{
    struct CaptureResource
    {
    public:
        static std::unique_ptr<CaptureResource> create(size_t deviceIndex, const CaptureOptions& options);
        ~CaptureResource();

        // Blocks (up to 500ms) grabbing the next frame from the camera and converts it into
        // m_pixels. This is the only place that talks to the underlying provider; loadPixels() and
        // loadTexture() are pure reads of whatever update() last produced, so calling either (or
        // both) any number of times between two update() calls never re-triggers camera I/O.
        bool update();

        std::optional<ReadOnlyPixels> loadPixels();
        std::optional<Texture> loadTexture();

        std::span<const WebcamResolution> getSupportedResolutions();
        void close();

    private:
        explicit CaptureResource(CcapProvider* provider, bool flipHorizontal);

        CcapProvider* m_provider;

        std::unique_ptr<GpuPixelStream> m_gpuPixelStream;
        Pixels m_pixels;
        // CpuPixelStream m_cpuPixelStream;

        std::vector<uint8_t> m_packedFrameBuffer;
        std::vector<WebcamResolution> m_supportedResolutions;
        bool m_flipHorizontal;

        bool m_hasFrame = false; // whether update() has ever produced a frame
        bool m_textureDirty = false; // whether m_pixels changed since the GPU stream last saw it
    };
} // namespace p5::webcam
