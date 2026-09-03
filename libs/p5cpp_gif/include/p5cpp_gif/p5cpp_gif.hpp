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
    //
    // Performance note: frame capture reads back the framebuffer via glReadPixels which blocks the GPU
    // pipeline on each capture. To minimize stutter during playback while recording, use a lower frame
    // rate (e.g. 10–15 fps) for long or large-canvas recordings. A 400×400 canvas at 30fps performs
    // ~60 readbacks/sec over PCIe — noticeable stutter. At 15fps the stutter drops by half.
    //
    // Returns false (and logs an error) if a recording is already in progress, path/duration/frame rate
    // are invalid, or createGIFRecorderPlugin() was never registered in the sketch's plugin list.
    bool saveGif(const std::filesystem::path& path, float recordingDurationInSeconds, int frameRatePerSecond = 15);

    std::unique_ptr<Plugin> createGIFRecorderPlugin();
} // namespace p5::gif
