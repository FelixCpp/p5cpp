#pragma once

#include <p5cpp/p5cpp.hpp>

#include <p5cpp_gif/gif_recording_resource.hpp>

namespace p5::gif
{
    class GifRecorder
    {
    public:
        GifRecorder();

        std::shared_ptr<GifRecordingResource> insertRecording(const std::filesystem::path& filepath, const GifStopCondition& condition, const GifRecordingOptions& options);
        void updateRecordings();
        void drawRecordingOverlay();

        void setOverlayCallback(GifRecordingOverlayCallback callback);

    private:
        void refreshCaptureSnapshot();

        Graphics m_captureGraphics;
        std::vector<std::shared_ptr<GifRecordingResource>> m_recordings;
        GifRecordingOverlayCallback m_overlayCallback;
    };
} // namespace p5::gif
