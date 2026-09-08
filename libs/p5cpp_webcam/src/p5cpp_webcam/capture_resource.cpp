#include <p5cpp_webcam/capture_resource.hpp>

#include <ccap_convert_c.h>

#include <algorithm>
#include <cmath>

namespace p5::webcam
{
    std::unique_ptr<CaptureResource> CaptureResource::create(size_t deviceIndex, const CaptureOptions& options)
    {
        CcapProvider* provider = ccap_provider_create();
        if (provider == nullptr) {
            error("openCapture(): failed to create the underlying CameraCapture provider");
            return nullptr;
        }

        ccap_provider_set_property(provider, CCAP_PROPERTY_PIXEL_FORMAT_OUTPUT, static_cast<double>(CCAP_PIXEL_FORMAT_BGRA32));

        if (options.requestedWidth > 0) {
            ccap_provider_set_property(provider, CCAP_PROPERTY_WIDTH, static_cast<double>(options.requestedWidth));
        }

        if (options.requestedHeight > 0) {
            ccap_provider_set_property(provider, CCAP_PROPERTY_HEIGHT, static_cast<double>(options.requestedHeight));
        }

        if (options.requestedFPS > 0.0f) {
            ccap_provider_set_property(provider, CCAP_PROPERTY_FRAME_RATE, static_cast<double>(options.requestedFPS));
        }

        if (not ccap_provider_open_by_index(provider, static_cast<int>(deviceIndex), true)) {
            error("openCapture(): failed to open camera at index {}", deviceIndex);
            ccap_provider_destroy(provider);
            return nullptr;
        }

        std::vector<WebcamResolution> supportedResolutions;
        CcapDeviceInfo deviceInfo;
        if (ccap_provider_get_device_info(provider, &deviceInfo)) {
            supportedResolutions.reserve(deviceInfo.resolutionCount);
            for (size_t i = 0; i < deviceInfo.resolutionCount; ++i) {
                supportedResolutions.push_back({
                    .width = deviceInfo.supportedResolutions[i].width,
                    .height = deviceInfo.supportedResolutions[i].height,
                });
            }
        }

        return std::unique_ptr<CaptureResource>(new CaptureResource(provider, std::move(supportedResolutions), options.flipHorizontal));
    }

    CaptureResource::CaptureResource(CcapProvider* provider, std::vector<WebcamResolution> supportedResolutions, bool flipHorizontal)
        : m_provider {provider},
          m_texture {},
          m_packedFrameBuffer {},
          m_supportedResolutions {std::move(supportedResolutions)},
          m_pendingWidth {0},
          m_pendingHeight {0},
          m_hasPendingFrame {false},
          m_flipHorizontal {flipHorizontal}
    {
    }

    CaptureResource::~CaptureResource()
    {
        close();
    }

    bool CaptureResource::isFrameNew()
    {
        pollLatestFrame();
        return m_hasPendingFrame;
    }

    Texture CaptureResource::getTexture()
    {
        pollLatestFrame();
        uploadPendingFrame();
        return m_texture;
    }

    Pixels CaptureResource::getPixels()
    {
        return getTexture().loadPixels();
    }

    float CaptureResource::getFPS() const
    {
        if (m_provider == nullptr) {
            return 0.0f;
        }

        const double fps = ccap_provider_get_property(m_provider, CCAP_PROPERTY_FRAME_RATE);
        return std::isnan(fps) ? 0.0f : static_cast<float>(fps);
    }

    std::span<const WebcamResolution> CaptureResource::getSupportedResolutions() const
    {
        return m_supportedResolutions;
    }

    void CaptureResource::close()
    {
        if (m_provider != nullptr) {
            ccap_provider_stop(m_provider);
            ccap_provider_close(m_provider);
            ccap_provider_destroy(m_provider);
            m_provider = nullptr;
        }
    }

    void CaptureResource::pollLatestFrame()
    {
        if (m_provider == nullptr) {
            return;
        }

        CcapVideoFrame* frame = ccap_provider_grab(m_provider, 0);
        if (frame == nullptr) {
            return;
        }

        CcapVideoFrameInfo info;
        if (not ccap_video_frame_get_info(frame, &info) or info.data[0] == nullptr) {
            ccap_video_frame_release(frame);
            return;
        }

        if (info.pixelFormat != CCAP_PIXEL_FORMAT_BGRA32 and info.pixelFormat != CCAP_PIXEL_FORMAT_RGBA32) {
            error("Webcam capture: unsupported pixel format 0x{:x} received from camera", static_cast<uint32_t>(info.pixelFormat));
            ccap_video_frame_release(frame);
            return;
        }

        const bool needsRowFlip = info.orientation != CCAP_FRAME_ORIENTATION_BOTTOM_TO_TOP;

        const size_t packedRowBytes = static_cast<size_t>(info.width) * 4;
        m_packedFrameBuffer.resize(packedRowBytes * info.height);

        const int passHeight = needsRowFlip ? -static_cast<int>(info.height) : static_cast<int>(info.height);
        if (info.pixelFormat == CCAP_PIXEL_FORMAT_BGRA32) {
            ccap_convert_bgra_to_rgba(info.data[0], static_cast<int>(info.stride[0]), m_packedFrameBuffer.data(), static_cast<int>(packedRowBytes), static_cast<int>(info.width), passHeight);
        } else {
            for (uint32_t destRow = 0; destRow < info.height; ++destRow) {
                const uint32_t srcRow = needsRowFlip ? (info.height - 1 - destRow) : destRow;
                std::copy_n(info.data[0] + static_cast<size_t>(srcRow) * info.stride[0], packedRowBytes, m_packedFrameBuffer.data() + static_cast<size_t>(destRow) * packedRowBytes);
            }
        }

        if (m_flipHorizontal) {
            for (uint32_t row = 0; row < info.height; ++row) {
                uint8_t* rowData = m_packedFrameBuffer.data() + static_cast<size_t>(row) * packedRowBytes;
                for (uint32_t x = 0; x < info.width / 2; ++x) {
                    std::swap_ranges(rowData + x * 4, rowData + x * 4 + 4, rowData + (info.width - 1 - x) * 4);
                }
            }
        }

        m_pendingWidth = info.width;
        m_pendingHeight = info.height;
        m_hasPendingFrame = true;

        ccap_video_frame_release(frame);
    }

    void CaptureResource::uploadPendingFrame()
    {
        if (not m_hasPendingFrame) {
            return;
        }

        if (not m_texture.isValid() or m_texture.size.x != m_pendingWidth or m_texture.size.y != m_pendingHeight) {
            std::optional<Texture> recreated = loadTexture(m_pendingWidth, m_pendingHeight, m_packedFrameBuffer, TexturePixelFormat::rgba8);
            if (not recreated.has_value()) {
                error("Webcam capture: failed to (re)create the frame texture");
                m_hasPendingFrame = false;
                return;
            }
            m_texture = std::move(recreated).value();
        } else {
            m_texture.updateSubImage(0, 0, m_pendingWidth, m_pendingHeight, m_packedFrameBuffer);
        }

        m_hasPendingFrame = false;
    }
} // namespace p5::webcam
