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
    // Performance note: frame capture reads the framebuffer back asynchronously (via
    // requestPixelReadback()/pollPixelReadback(), backed by a small ring of fenced Pixel Buffer
    // Objects), so it does not stall the render thread the way a direct
    // glGetTexImage()/loadPixels() call would. In exchange, a
    // captured frame reaches the encoder roughly one to a few frames after it was requested; on a
    // sufficiently overloaded GPU/large canvas, an undrained readback can be dropped rather than
    // captured (a frame is lost, but the render thread still never blocks). The recording still
    // finishes only once every requested frame has actually been drained, so the file's total
    // length is unaffected -- only the tail end lands slightly after recordingDurationInSeconds
    // elapses.
    //
    // Returns false (and logs an error) if a recording is already in progress, path/duration/frame rate
    // are invalid, or createGIFRecorderPlugin() was never registered in the sketch's plugin list.
    bool saveGif(const std::filesystem::path& path, float recordingDurationInSeconds, int frameRatePerSecond = 15);

    std::unique_ptr<Plugin> createGIFRecorderPlugin();
} // namespace p5::gif
