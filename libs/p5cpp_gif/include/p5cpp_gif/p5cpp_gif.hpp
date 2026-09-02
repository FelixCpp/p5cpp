#pragma once

#include <p5cpp/p5cpp.hpp>

namespace p5::gif
{
    // Records the canvas for recordingDurationInSeconds and writes it to path as an animated GIF,
    // sampling one frame every 1/frameRatePerSecond seconds. Mirrors p5.js's saveGif(), except it
    // writes straight to a file instead of triggering a browser download.
    //
    // Call this once (e.g. on a key press) to arm a recording; the actual capturing happens over the
    // following frames' draw() calls, and the GIF is encoded and flushed to path once the duration has
    // elapsed. Returns true once the recording has been armed, not once the file has finished writing.
    // Returns false (and logs an error) if a recording is already in progress, path/duration/frame rate
    // are invalid, or createGIFRecorderPlugin() was never registered in the sketch's plugin list.
    bool saveGif(const std::filesystem::path& path, float recordingDurationInSeconds, int frameRatePerSecond = 30);

    std::unique_ptr<Plugin> createGIFRecorderPlugin();
} // namespace p5::gif
