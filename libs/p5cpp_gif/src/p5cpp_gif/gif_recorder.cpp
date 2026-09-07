#include <p5cpp_gif/gif_recorder.hpp>

namespace p5::gif
{
    namespace
    {
        template <typename... T>
        struct visitors : T...
        {
            using T::operator()...;
        };
    } // namespace

    GifRecorder::GifRecorder()
        : m_captureGraphics {},
          m_recordings {},
          m_overlayCallback {&defaultGifRecordingOverlay}
    {
    }

    std::shared_ptr<GifRecordingResource> GifRecorder::insertRecording(const std::filesystem::path& filepath, const GifStopCondition& condition, const GifRecordingOptions& options)
    {
        if (options.framesPerSecond <= 0.0f) {
            error("recordGif(): framesPerSecond must be > 0, got {}", options.framesPerSecond);
            return nullptr;
        }

        const bool isConditionValid = std::visit(
            visitors {
                [](const RecordForFrameCount& c) {
                    return c.frameCount > 0;
                },
                [](const RecordForSeconds& c) {
                    return c.seconds > 0.0f;
                },
                [](const RecordUntil& c) {
                    return static_cast<bool>(c.condition);
                },
            },
            condition
        );

        if (not isConditionValid) {
            error("recordGif(): stop condition is invalid (zero frame count/duration, or an empty predicate)");
            return nullptr;
        }

        std::shared_ptr<GifRecordingResource> recording = GifRecordingResource::create(filepath, condition, options, getWidth(), getHeight());
        if (recording == nullptr) {
            return nullptr;
        }

        m_recordings.push_back(recording);
        return recording;
    }

    void GifRecorder::updateRecordings()
    {
        refreshCaptureSnapshot();

        const float deltaTimeInSeconds = static_cast<float>(getDeltaTime());

        for (auto itr = m_recordings.begin(); itr != m_recordings.end();) {
            std::shared_ptr<GifRecordingResource>& recording = *itr;
            recording->update(deltaTimeInSeconds, m_captureGraphics);

            if (recording->isRecordingComplete()) {
                itr = m_recordings.erase(itr);
            } else {
                ++itr;
            }
        }
    }

    void GifRecorder::drawRecordingOverlay()
    {
        if (m_overlayCallback == nullptr) {
            return;
        }

        std::vector<GifRecordingStatus> statuses;
        statuses.reserve(m_recordings.size());

        for (const auto& recording : m_recordings) {
            statuses.push_back({
                .label = recording->getLabel(),
                .progress = recording->getProgress(),
            });
        }

        m_overlayCallback(statuses);
    }

    void GifRecorder::setOverlayCallback(GifRecordingOverlayCallback callback)
    {
        m_overlayCallback = std::move(callback);
    }

    void GifRecorder::refreshCaptureSnapshot()
    {
        if (m_recordings.empty()) {
            return;
        }

        Graphics live = peekGraphics();
        if (not live.isValid()) {
            error("GIF recording: refreshCaptureSnapshot() called with no graphics pushed");
            return;
        }

        if (not m_captureGraphics.isValid() or m_captureGraphics.size != live.size) {
            std::optional<Graphics> recreated = createGraphics(live.size.x, live.size.y);
            if (not recreated.has_value()) {
                error("GIF recording: failed to (re)create the capture snapshot target");
                return;
            }
            m_captureGraphics = std::move(recreated).value();
        }

        withGraphics(
            m_captureGraphics,
            [&live] {
                image(live, 0.0f, 0.0f, static_cast<float>(live.size.x), static_cast<float>(live.size.y));
            },
            false
        );
    }
} // namespace p5::gif
