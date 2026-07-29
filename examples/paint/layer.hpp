#pragma once

#include <p5cpp/p5cpp.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace paint
{
    struct Layer
    {
        p5cpp::Framebuffer framebuffer;
        std::string name;
        float opacity = 1.0f;
        bool visible = true;
    };

    // Per-layer pixel snapshot used by the undo/redo stack (history.hpp) — owns
    // its own copy of the pixel data so it survives independently of the live Layer.
    struct LayerSnapshot
    {
        p5cpp::Pixels pixels;
        std::string name;
        float opacity;
        bool visible;
    };

    struct HistoryState
    {
        std::vector<LayerSnapshot> layers;
        size_t activeLayerIndex = 0;
    };

    // Owns the stack of layers that make up the painting, plus the pan/zoom
    // mapping between window/screen pixels and canvas pixels. Layers are drawn
    // in vector order (index 0 first / bottom, last index on top).
    class Canvas
    {
    public:
        Canvas(uint32_t width, uint32_t height);
        ~Canvas();

        uint32_t getWidth() const { return m_width; }
        uint32_t getHeight() const { return m_height; }

        std::vector<Layer>& layers() { return m_layers; }
        const std::vector<Layer>& layers() const { return m_layers; }

        size_t activeLayerIndex() const { return m_activeLayerIndex; }
        void setActiveLayerIndex(size_t index);
        Layer& activeLayer() { return m_layers[m_activeLayerIndex]; }
        const Layer& activeLayer() const { return m_layers[m_activeLayerIndex]; }

        Layer& addLayer(const std::string& name);
        void removeLayer(size_t index);
        void moveLayerUp(size_t index);   // towards the end of the vector / top of the stack
        void moveLayerDown(size_t index); // towards the start of the vector / bottom of the stack

        // Screen-space (window pixel) <-> canvas-space (painting pixel) mapping
        // for the current pan/zoom and viewport origin.
        p5cpp::float2 screenToCanvas(float screenX, float screenY) const;
        p5cpp::float2 canvasToScreen(float canvasX, float canvasY) const;

        // Wraps drawing that should follow the canvas's pan/zoom transform (the
        // composited layers themselves, and any tool preview overlay).
        void pushCanvasTransform() const;
        void popCanvasTransform() const;

        float viewportX = 0.0f;
        float viewportY = 0.0f;
        float panX = 0.0f;
        float panY = 0.0f;
        float zoom = 1.0f;

        // Draws every visible layer (respecting opacity), wrapped in its own
        // pushCanvasTransform()/popCanvasTransform().
        void compositeToScreen() const;

        // Flattens all visible layers (respecting opacity) into a fresh
        // Framebuffer at canvas resolution, e.g. for saveImage().
        p5cpp::Framebuffer flattenToFramebuffer() const;

        HistoryState captureState() const;
        void restoreState(const HistoryState& state);

    private:
        uint32_t m_width;
        uint32_t m_height;
        std::vector<Layer> m_layers;
        size_t m_activeLayerIndex = 0;
    };

    // Flood-fills `pixels` starting at (x, y) with `fillColor`, treating pixels
    // within `tolerance` (per-channel abs diff) of the seed color as matching —
    // a plain equality check would leave a halo of anti-aliased edge pixels
    // unfilled on vector-drawn strokes/shapes.
    void floodFill(p5cpp::Pixels& pixels, int x, int y, p5cpp::color_t fillColor, int tolerance = 32);

    // loadTexture()/Texture::upload() expect bottom-to-top GL row order, but
    // Pixels (top-left origin, like everywhere else in p5cpp) is not — flip
    // once when handing hand-built pixel data to the GPU, or it comes out
    // vertically mirrored. Used for anything built as a Pixels buffer and then
    // uploaded via loadTexture() (selection preview, color-picker gradients, ...).
    std::vector<p5cpp::color_t> flippedRows(const p5cpp::Pixels& pixels);
} // namespace paint
