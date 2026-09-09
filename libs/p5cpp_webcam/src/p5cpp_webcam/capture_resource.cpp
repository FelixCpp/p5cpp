#include <algorithm>
#include <p5cpp_webcam/capture_resource.hpp>

#include <ccap_c.h>
#include <ccap_convert_c.h>

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
        ccap_provider_set_property(provider, CCAP_PROPERTY_FRAME_ORIENTATION, static_cast<double>(CCAP_FRAME_ORIENTATION_BOTTOM_TO_TOP));

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

        return std::unique_ptr<CaptureResource>(new CaptureResource(provider, options.flipHorizontal, options.syncToGpuTexture));
    }

    CaptureResource::~CaptureResource()
    {
        close();
    }

    const Pixels& CaptureResource::loadPixels()
    {
        CcapVideoFrame* frame = ccap_provider_grab(m_provider, 100);
        if (frame == nullptr) {
            error("CaptureResource::loadPixels(): failed to grab a frame from the underlying CameraCapture provider");
            return m_pixels;
        }

        CcapVideoFrameInfo frameInfo;
        if (not ccap_video_frame_get_info(frame, &frameInfo)) {
            error("CaptureResource::loadPixels(): failed to get frame info from the underlying CameraCapture provider");
            return m_pixels;
        }

        if (frameInfo.pixelFormat != CCAP_PIXEL_FORMAT_BGRA32) {
            error("CaptureResource::loadPixels(): unexpected pixel format from the underlying CameraCapture provider");
            return m_pixels;
        }

        const int width = static_cast<int>(frameInfo.width);
        const int height = static_cast<int>(frameInfo.height);

        const int stride = frameInfo.stride[0];
        const uint8_t* rawData = frameInfo.data[0];
        const int packedRowBytes = width * 4;

        m_pixels.width = width;
        m_pixels.height = height;
        m_pixels.data.resize(static_cast<size_t>(packedRowBytes) * static_cast<size_t>(height));

        // Convert straight into the tightly-packed destination buffer (dst stride = packedRowBytes,
        // independent of the source's possibly-padded stride) and flip vertically in the same pass
        // (negative height) so the camera's bottom-to-top frame ends up top-down, matching the rest
        // of the engine's Pixels convention.
        ccap_convert_bgra_to_rgba(rawData, stride, m_pixels.data.data(), packedRowBytes, width, -height);

        if (m_flipHorizontal) {
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width / 2; ++x) {
                    const int leftIndex = y * packedRowBytes + x * 4;
                    const int rightIndex = y * packedRowBytes + (width - 1 - x) * 4;
                    std::swap_ranges(m_pixels.data.begin() + leftIndex, m_pixels.data.begin() + leftIndex + 4, m_pixels.data.begin() + rightIndex);
                }
            }
        }

        ccap_video_frame_release(frame);

        return m_pixels;
    }

    std::span<const WebcamResolution> CaptureResource::getSupportedResolutions()
    {
        if (m_supportedResolutions.empty()) {
            CcapDeviceInfo deviceInfo;
            if (not ccap_provider_get_device_info(m_provider, &deviceInfo)) {
                error("CaptureResource::getSupportedResolutions(): failed to get device info from the underlying CameraCapture provider");
                return {};
            }

            m_supportedResolutions.reserve(deviceInfo.resolutionCount);

            for (size_t i = 0; i < deviceInfo.resolutionCount; ++i) {
                m_supportedResolutions.push_back({
                    .width = deviceInfo.supportedResolutions[i].width,
                    .height = deviceInfo.supportedResolutions[i].height,
                });
            }
        }

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

    CaptureResource::CaptureResource(CcapProvider* provider, bool flipHorizontal, bool syncToGpuTexture)
        : m_provider {provider},
          m_gpuPixelStream(syncToGpuTexture ? std::make_unique<GpuPixelStream>() : nullptr),
          m_packedFrameBuffer {},
          m_flipHorizontal {flipHorizontal}
    {
    }
} // namespace p5::webcam
