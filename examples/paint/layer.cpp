#include "layer.hpp"

#include <algorithm>
#include <cstdlib>
#include <utility>

using namespace p5cpp;

namespace paint
{
    Canvas::Canvas(uint32_t width, uint32_t height)
        : m_width(width), m_height(height)
    {
        Layer& background = addLayer("Background");
        pushCanvas(background.framebuffer);
            p5cpp::background(255);
        popCanvas();
    }

    void Canvas::setActiveLayerIndex(size_t index)
    {
        if (index < m_layers.size()) {
            m_activeLayerIndex = index;
        }
    }

    Layer& Canvas::addLayer(const std::string& name)
    {
        Layer layer;
        layer.framebuffer = createFramebuffer(m_width, m_height);
        layer.name = name;

        pushCanvas(layer.framebuffer);
            p5cpp::background(0, 0); // transparent
        popCanvas();

        m_layers.push_back(std::move(layer));
        m_activeLayerIndex = m_layers.size() - 1;
        return m_layers.back();
    }

    void Canvas::removeLayer(size_t index)
    {
        if (m_layers.size() <= 1 || index >= m_layers.size()) {
            return;
        }

        m_layers.erase(m_layers.begin() + static_cast<std::ptrdiff_t>(index));

        if (m_activeLayerIndex >= m_layers.size()) {
            m_activeLayerIndex = m_layers.size() - 1;
        } else if (m_activeLayerIndex > index) {
            --m_activeLayerIndex;
        }
    }

    void Canvas::moveLayerUp(size_t index)
    {
        if (index + 1 >= m_layers.size()) {
            return;
        }

        std::swap(m_layers[index], m_layers[index + 1]);
        if (m_activeLayerIndex == index) {
            m_activeLayerIndex = index + 1;
        } else if (m_activeLayerIndex == index + 1) {
            m_activeLayerIndex = index;
        }
    }

    void Canvas::moveLayerDown(size_t index)
    {
        if (index == 0 || index >= m_layers.size()) {
            return;
        }

        std::swap(m_layers[index], m_layers[index - 1]);
        if (m_activeLayerIndex == index) {
            m_activeLayerIndex = index - 1;
        } else if (m_activeLayerIndex == index - 1) {
            m_activeLayerIndex = index;
        }
    }

    float2 Canvas::screenToCanvas(float screenX, float screenY) const
    {
        return float2 {
            (screenX - viewportX - panX) / zoom,
            (screenY - viewportY - panY) / zoom,
        };
    }

    float2 Canvas::canvasToScreen(float canvasX, float canvasY) const
    {
        return float2 {
            canvasX * zoom + panX + viewportX,
            canvasY * zoom + panY + viewportY,
        };
    }

    void Canvas::pushCanvasTransform() const
    {
        pushMatrix();
        translate(viewportX + panX, viewportY + panY);
        scale(zoom, zoom);
    }

    void Canvas::popCanvasTransform() const
    {
        popMatrix();
    }

    void Canvas::compositeToScreen() const
    {
        pushCanvasTransform();
        for (const Layer& layer : m_layers) {
            if (not layer.visible) {
                continue;
            }
            tint(255, 255, 255, static_cast<int>(layer.opacity * 255.0f));
            image(layer.framebuffer.colorTexture, 0, 0, static_cast<float>(m_width), static_cast<float>(m_height));
        }
        noTint();
        popCanvasTransform();
    }

    Framebuffer Canvas::flattenToFramebuffer() const
    {
        Framebuffer result = createFramebuffer(m_width, m_height);

        pushCanvas(result);
            p5cpp::background(0, 0);
            for (const Layer& layer : m_layers) {
                if (not layer.visible) {
                    continue;
                }
                tint(255, 255, 255, static_cast<int>(layer.opacity * 255.0f));
                image(layer.framebuffer.colorTexture, 0, 0, static_cast<float>(m_width), static_cast<float>(m_height));
            }
            noTint();
        popCanvas();

        return result;
    }

    HistoryState Canvas::captureState() const
    {
        HistoryState state;
        state.activeLayerIndex = m_activeLayerIndex;
        state.layers.reserve(m_layers.size());

        for (const Layer& layer : m_layers) {
            pushCanvas(layer.framebuffer);
                Pixels pixels = loadPixels();
            popCanvas();

            state.layers.push_back(LayerSnapshot {std::move(pixels), layer.name, layer.opacity, layer.visible});
        }

        return state;
    }

    void Canvas::restoreState(const HistoryState& state)
    {
        if (m_layers.size() > state.layers.size()) {
            m_layers.resize(state.layers.size());
        } else {
            while (m_layers.size() < state.layers.size()) {
                Layer layer;
                layer.framebuffer = createFramebuffer(m_width, m_height);
                m_layers.push_back(std::move(layer));
            }
        }

        for (size_t i = 0; i < state.layers.size(); ++i) {
            Layer& layer = m_layers[i];
            const LayerSnapshot& snapshot = state.layers[i];
            layer.name = snapshot.name;
            layer.opacity = snapshot.opacity;
            layer.visible = snapshot.visible;

            pushCanvas(layer.framebuffer);
                updatePixels(snapshot.pixels);
            popCanvas();
        }

        m_activeLayerIndex = std::min(state.activeLayerIndex, m_layers.size() - 1);
    }

    void floodFill(Pixels& pixels, int x, int y, color_t fillColor, int tolerance)
    {
        const uint32_t width = pixels.width;
        const uint32_t height = pixels.height;

        if (x < 0 or y < 0 or static_cast<uint32_t>(x) >= width or static_cast<uint32_t>(y) >= height) {
            return;
        }

        const color_t seedColor = get(pixels, static_cast<uint32_t>(x), static_cast<uint32_t>(y));
        if (seedColor == fillColor) {
            return;
        }

        const auto matches = [&](color_t c) {
            return std::abs(red(c) - red(seedColor)) <= tolerance
                and std::abs(green(c) - green(seedColor)) <= tolerance
                and std::abs(blue(c) - blue(seedColor)) <= tolerance
                and std::abs(alpha(c) - alpha(seedColor)) <= tolerance;
        };

        std::vector<bool> visited(static_cast<size_t>(width) * height, false);
        std::vector<std::pair<int, int>> stack;
        stack.emplace_back(x, y);

        while (not stack.empty()) {
            const auto [cx, cy] = stack.back();
            stack.pop_back();

            if (cx < 0 or cy < 0 or static_cast<uint32_t>(cx) >= width or static_cast<uint32_t>(cy) >= height) {
                continue;
            }

            const size_t index = static_cast<size_t>(cy) * width + static_cast<uint32_t>(cx);
            if (visited[index]) {
                continue;
            }
            if (not matches(get(pixels, static_cast<uint32_t>(cx), static_cast<uint32_t>(cy)))) {
                continue;
            }

            visited[index] = true;
            set(pixels, static_cast<uint32_t>(cx), static_cast<uint32_t>(cy), fillColor);

            stack.emplace_back(cx + 1, cy);
            stack.emplace_back(cx - 1, cy);
            stack.emplace_back(cx, cy + 1);
            stack.emplace_back(cx, cy - 1);
        }
    }

    std::vector<color_t> flippedRows(const Pixels& pixels)
    {
        const uint32_t width = pixels.width;
        const uint32_t height = pixels.height;
        std::vector<color_t> flipped(static_cast<size_t>(width) * height);
        for (uint32_t y = 0; y < height; ++y) {
            const uint32_t srcY = height - 1 - y;
            for (uint32_t x = 0; x < width; ++x) {
                flipped[static_cast<size_t>(y) * width + x] = get(pixels, x, srcY);
            }
        }
        return flipped;
    }
} // namespace paint
