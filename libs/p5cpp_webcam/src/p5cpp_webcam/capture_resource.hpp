#pragma once

#include <p5cpp/p5cpp.hpp>
#include <p5cpp_webcam/p5cpp_webcam.hpp>

#include <ccap_c.h>

#include <vector>

namespace p5::webcam
{
    struct CaptureResource
    {
    public:
        static std::unique_ptr<CaptureResource> create(size_t deviceIndex, const CaptureOptions& options);
        ~CaptureResource();

        CaptureResource(const CaptureResource&) = delete;
        CaptureResource& operator=(const CaptureResource&) = delete;

        bool isFrameNew();
        Texture getTexture();
        Pixels getPixels();
        float getFPS() const;
        std::span<const WebcamResolution> getSupportedResolutions() const;
        void close();

    private:
        CaptureResource(CcapProvider* provider, std::vector<WebcamResolution> supportedResolutions, bool flipHorizontal);

        void pollLatestFrame();
        void uploadPendingFrame();

        CcapProvider* m_provider;
        Texture m_texture;
        std::vector<uint8_t> m_packedFrameBuffer;
        std::vector<WebcamResolution> m_supportedResolutions;
        uint32_t m_pendingWidth;
        uint32_t m_pendingHeight;
        bool m_hasPendingFrame;
        bool m_flipHorizontal;
    };
} // namespace p5::webcam
