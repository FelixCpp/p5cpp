#include <p5cpp_gif/p5cpp_gif.hpp>
#include <p5cpp_gif/gif_recorder.hpp>

#include <cmath>

namespace p5::gif
{
    inline static thread_local std::unique_ptr<GifRecorder> recorder;
} // namespace p5::gif

namespace p5::gif
{
    bool GifRecording::isValid() const
    {
        return resource != nullptr;
    }

    bool GifRecording::isActive() const
    {
        return resource != nullptr and not resource->isRecordingComplete();
    }

    std::optional<float> GifRecording::getProgress() const
    {
        return resource != nullptr ? resource->getProgress() : std::nullopt;
    }

    void GifRecording::cancel()
    {
        if (resource != nullptr) {
            resource->cancel();
        }
    }

    std::optional<GifRecording> recordGif(const std::filesystem::path& path, const GifStopCondition& condition, const GifRecordingOptions& options)
    {
        if (recorder == nullptr) {
            error("GIFRecorder is not initialized. Please add the GIFRecorderPlugin to your sketch.");
            return std::nullopt;
        }

        std::shared_ptr<GifRecordingResource> resource = recorder->insertRecording(path, condition, options);
        if (resource == nullptr) {
            return std::nullopt;
        }

        return GifRecording {.resource = std::move(resource)};
    }

    void setGifRecordingOverlayCallback(GifRecordingOverlayCallback callback)
    {
        if (recorder == nullptr) {
            error("GIFRecorder is not initialized. Please add the GIFRecorderPlugin to your sketch.");
            return;
        }

        recorder->setOverlayCallback(std::move(callback));
    }
} // namespace p5::gif

namespace p5::gif
{
    class GIFRecorderPlugin : public Plugin
    {
    public:
        void setup(const Next& next) override
        {
            recorder = std::make_unique<GifRecorder>();
            provideDependency(recorder.get());

            next();
        }

        void draw(const Next& next) override
        {
            next();

            recorder->updateRecordings();
            recorder->drawRecordingOverlay();
        }

        void destroy(const Next& next) override
        {
            next();

            removeDependency<GifRecorder>();
            recorder.reset();
        }
    };
} // namespace p5::gif

namespace p5::gif
{
    std::unique_ptr<Plugin> createGifRecorderPlugin()
    {
        return std::make_unique<GIFRecorderPlugin>();
    }
} // namespace p5::gif

namespace p5::gif
{
    void defaultGifRecordingOverlay(std::span<const GifRecordingStatus> recordings)
    {
        if (recordings.empty()) {
            return;
        }

        static constexpr float margin = 16.0f;
        static constexpr float badgeHeight = 28.0f;
        static constexpr float badgeSpacing = 8.0f;
        static constexpr float horizontalPadding = 14.0f;

        textAlign(TextAlignment::centerLeft);
        textSize(13.0f);
        noStroke();

        const uint2 canvasSize = getGraphicsSize();
        const float pulse = 0.5f + 0.5f * std::sin(getGlobalTime() * 6.0f);

        float y = margin;
        for (const GifRecordingStatus& recording : recordings) {
            const std::string& label = recording.label;

            const float dotDiameter = badgeHeight * 0.4f;
            const float badgeWidth = horizontalPadding + dotDiameter + 8.0f + textWidth(label) + horizontalPadding;
            const float x = static_cast<float>(canvasSize.x) - margin - badgeWidth;

            // Backdrop pill so the badge stays legible over any sketch content.
            fill(rgba(20, 20, 20, 190));
            rect(x, y, badgeWidth, badgeHeight, BorderRadius::all(badgeHeight * 0.5f));

            // Pulsing "recording" dot.
            fill(rgba(255, static_cast<int32_t>(lerp(70.0f, 110.0f, pulse)), static_cast<int32_t>(lerp(70.0f, 110.0f, pulse))));
            circle(x + horizontalPadding + dotDiameter * 0.5f, y + badgeHeight * 0.5f, dotDiameter);

            // Filename label.
            fill(rgba(255));
            text(label, x + horizontalPadding + dotDiameter + 8.0f, y + badgeHeight * 0.5f);

            // Thin progress bar along the bottom edge of the pill.
            const float barLeft = x + horizontalPadding * 0.5f;
            const float barWidth = badgeWidth - horizontalPadding;
            fill(rgba(255, 255, 255, 60));
            rect(barLeft, y + badgeHeight - 4.0f, barWidth, 2.0f);
            fill(rgba(255, 90, 90));

            if (recording.progress.has_value()) {
                rect(barLeft, y + badgeHeight - 4.0f, barWidth * *recording.progress, 2.0f);
            } else {
                // RecordUntil has no known endpoint -- a sliding marquee segment instead of a fake
                // percentage, so it's honest about "running, but no ETA" rather than lying.
                const float segmentWidth = barWidth * 0.25f;
                const float t = 0.5f + 0.5f * std::sin(getGlobalTime() * 3.0f);
                rect(barLeft + (barWidth - segmentWidth) * t, y + badgeHeight - 4.0f, segmentWidth, 2.0f);
            }

            y += badgeHeight + badgeSpacing;
        }
    }
} // namespace p5::gif
