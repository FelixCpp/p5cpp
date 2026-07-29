#include <p5cpp/graphics/pixels.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <utility>

namespace p5cpp
{
    Pixels::Pixels()
        : m_width(0), m_height(0)
    {
    }

    Pixels::Pixels(uint32_t width, uint32_t height)
        : m_width(width), m_height(height), m_data(static_cast<size_t>(width) * height)
    {
    }

    Pixels::Pixels(uint32_t width, uint32_t height, std::vector<color_t> data)
        : m_width(width), m_height(height), m_data(std::move(data))
    {
        assert(m_data.size() == static_cast<size_t>(width) * height);
    }

    uint2 Pixels::getSize() const { return uint2(m_width, m_height); }
    uint32_t Pixels::getWidth() const { return m_width; }
    uint32_t Pixels::getHeight() const { return m_height; }
    size_t Pixels::size() const { return m_data.size(); }

    color_t& Pixels::operator[](size_t index) { return m_data[index]; }
    color_t Pixels::operator[](size_t index) const { return m_data[index]; }

    color_t Pixels::get(uint32_t x, uint32_t y) const { return m_data[static_cast<size_t>(y) * m_width + x]; }
    void Pixels::set(uint32_t x, uint32_t y, color_t color) { m_data[static_cast<size_t>(y) * m_width + x] = color; }

    color_t* Pixels::data() { return m_data.data(); }
    const color_t* Pixels::data() const { return m_data.data(); }

    std::vector<color_t>::iterator Pixels::begin() { return m_data.begin(); }
    std::vector<color_t>::iterator Pixels::end() { return m_data.end(); }
    std::vector<color_t>::const_iterator Pixels::begin() const { return m_data.begin(); }
    std::vector<color_t>::const_iterator Pixels::end() const { return m_data.end(); }
} // namespace p5cpp

namespace p5cpp::detail
{
    std::vector<color_t> flipRows(std::span<const color_t> src, uint32_t width, uint32_t height)
    {
        std::vector<color_t> flipped(src.size());
        for (uint32_t y = 0; y < height; ++y) {
            const size_t srcRow = static_cast<size_t>(height - 1 - y) * width;
            const size_t dstRow = static_cast<size_t>(y) * width;
            std::copy_n(src.begin() + static_cast<ptrdiff_t>(srcRow), width, flipped.begin() + static_cast<ptrdiff_t>(dstRow));
        }
        return flipped;
    }
} // namespace p5cpp::detail
