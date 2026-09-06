#pragma once

#include <p5cpp/p5cpp.hpp>

#include <functional>
#include <optional>
#include <span>
#include <variant>

namespace p5::gif
{

    struct RecordForFrameCount
    {
        size_t frameCount;
    };

    struct RecordForSeconds
    {
        float seconds;
    };

    using RecordUntilCondition = std::function<bool(float elapsedTimeInSeconds)>;

    struct RecordUntil
    {
        RecordUntilCondition condition;
    };

    using GifStopCondition = std::variant<RecordForFrameCount, RecordForSeconds, RecordUntil>;

    constexpr RecordForFrameCount recordForFrames(size_t frameCount);
    constexpr RecordForSeconds recordForSeconds(float seconds);
    RecordUntil recordUntil(RecordUntilCondition condition);

    struct GifRecordingOptions
    {
        float framesPerSecond = 30.0f;
    };

    struct GifRecordingResource;
    struct GifRecording
    {
        std::shared_ptr<GifRecordingResource> resource;

        bool operator==(const GifRecording&) const = default;
        bool isValid() const;
        bool isActive() const;
        std::optional<float> getProgress() const;
        void cancel();
    };

    std::optional<GifRecording> recordGif(const std::filesystem::path& path, const GifStopCondition& condition, const GifRecordingOptions& options = {});
    std::unique_ptr<Plugin> createGIFRecorderPlugin();

    struct GifRecordingStatus
    {
        std::string label;
        std::optional<float> progress; // nullopt for RecordUntil recordings -- no known endpoint
    };

    using GifRecordingOverlayCallback = std::function<void(std::span<const GifRecordingStatus> recordings)>;
    void defaultGifRecordingOverlay(std::span<const GifRecordingStatus> recordings);
    void setGifRecordingOverlayCallback(GifRecordingOverlayCallback callback);
} // namespace p5::gif

namespace p5::gif
{
    inline constexpr RecordForFrameCount recordForFrames(size_t frameCount)
    {
        return {.frameCount = frameCount};
    }

    inline constexpr RecordForSeconds recordForSeconds(float seconds)
    {
        return {.seconds = seconds};
    }

    inline RecordUntil recordUntil(RecordUntilCondition condition)
    {
        return {.condition = std::move(condition)};
    }
} // namespace p5::gif
