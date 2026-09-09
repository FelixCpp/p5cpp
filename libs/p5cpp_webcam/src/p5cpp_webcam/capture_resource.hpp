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

        const Pixels& loadPixels();

        std::span<const WebcamResolution> getSupportedResolutions();
        void close();

    private:
        explicit CaptureResource(CcapProvider* provider, bool flipHorizontal, bool syncToGpuTexture);

        CcapProvider* m_provider;

        std::unique_ptr<GpuPixelStream> m_gpuPixelStream;
        Pixels m_pixels;
        // CpuPixelStream m_cpuPixelStream;

        std::vector<uint8_t> m_packedFrameBuffer;
        std::vector<WebcamResolution> m_supportedResolutions;
        bool m_flipHorizontal;
    };
} // namespace p5::webcam
