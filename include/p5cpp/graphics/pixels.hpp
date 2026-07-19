#pragma once

#include <p5cpp/graphics/color.hpp>
#include <p5cpp/math/value2.hpp>

#include <cassert>
#include <cstddef>
#include <utility>
#include <vector>

namespace p5cpp
{
    // Top-left origin, row-major pixel buffer — matches the coordinate system used
    // everywhere else in p5cpp (e.g. rect()/image()), unlike Framebuffer::readPixels()/
    // writePixels() which use raw bottom-to-top GL row order.
    class Pixels
    {
    public:
        Pixels();
        Pixels(uint32_t width, uint32_t height);
        Pixels(uint32_t width, uint32_t height, std::vector<color_t> data);

        uint2 getSize() const;
        uint32_t getWidth() const;
        uint32_t getHeight() const;
        size_t size() const;

        color_t& operator[](size_t index);
        color_t operator[](size_t index) const;

        color_t get(uint32_t x, uint32_t y) const;
        void set(uint32_t x, uint32_t y, color_t color);

        color_t* data();
        const color_t* data() const;

        std::vector<color_t>::iterator begin();
        std::vector<color_t>::iterator end();
        std::vector<color_t>::const_iterator begin() const;
        std::vector<color_t>::const_iterator end() const;

    private:
        uint32_t m_width;
        uint32_t m_height;
        std::vector<color_t> m_data;
    };
} // namespace p5cpp

namespace p5cpp
{
    inline Pixels::Pixels()
        : m_width(0), m_height(0)
    {
    }

    inline Pixels::Pixels(uint32_t width, uint32_t height)
        : m_width(width), m_height(height), m_data(static_cast<size_t>(width) * height)
    {
    }

    inline Pixels::Pixels(uint32_t width, uint32_t height, std::vector<color_t> data)
        : m_width(width), m_height(height), m_data(std::move(data))
    {
        assert(m_data.size() == static_cast<size_t>(width) * height);
    }

    inline uint2 Pixels::getSize() const { return uint2(m_width, m_height); }
    inline uint32_t Pixels::getWidth() const { return m_width; }
    inline uint32_t Pixels::getHeight() const { return m_height; }
    inline size_t Pixels::size() const { return m_data.size(); }

    inline color_t& Pixels::operator[](size_t index) { return m_data[index]; }
    inline color_t Pixels::operator[](size_t index) const { return m_data[index]; }

    inline color_t Pixels::get(uint32_t x, uint32_t y) const { return m_data[static_cast<size_t>(y) * m_width + x]; }
    inline void Pixels::set(uint32_t x, uint32_t y, color_t color) { m_data[static_cast<size_t>(y) * m_width + x] = color; }

    inline color_t* Pixels::data() { return m_data.data(); }
    inline const color_t* Pixels::data() const { return m_data.data(); }

    inline std::vector<color_t>::iterator Pixels::begin() { return m_data.begin(); }
    inline std::vector<color_t>::iterator Pixels::end() { return m_data.end(); }
    inline std::vector<color_t>::const_iterator Pixels::begin() const { return m_data.begin(); }
    inline std::vector<color_t>::const_iterator Pixels::end() const { return m_data.end(); }
} // namespace p5cpp
