#pragma once

#include <p5cpp/math/value2.hpp>

#include <cstdint>
#include <optional>
#include <algorithm>

namespace p5cpp
{
    template <typename T>
    struct rectangle
    {
        constexpr rectangle();
        constexpr rectangle(T left, T top, T width, T height);

        template <typename U>
        constexpr rectangle(const rectangle<U>& other);

        constexpr T right() const;
        constexpr T bottom() const;

        constexpr bool contains(T x, T y) const;
        constexpr bool contains(const rectangle<T>& other) const;
        constexpr bool intersects(const rectangle<T>& other) const;
        constexpr std::optional<rectangle<T>> intersection(const rectangle<T>& other) const;

        constexpr bool operator==(const rectangle<T>& other) const;
        constexpr bool operator!=(const rectangle<T>& other) const;

        union
        {
            struct
            {
                T left, top;
            };

            value2<T> position;
        };

        union
        {
            struct
            {
                T width, height;
            };

            value2<T> size;
        };
    };

    typedef rectangle<float> float_rect;
    typedef rectangle<int32_t> int_rect;
    typedef rectangle<uint32_t> uint_rect;
} // namespace p5cpp

namespace p5cpp
{
    template <typename T> inline constexpr rectangle<T>::rectangle() : left(T {}), top(T {}), width(T {}), height(T {}) {}
    template <typename T> inline constexpr rectangle<T>::rectangle(T left, T top, T width, T height) : left(left), top(top), width(width), height(height) {}
    template <typename T> template <typename U> inline constexpr rectangle<T>::rectangle(const rectangle<U>& other) : left(static_cast<T>(other.left)), top(static_cast<T>(other.top)), width(static_cast<T>(other.width)), height(static_cast<T>(other.height)) {}
    template <typename T> inline constexpr T rectangle<T>::right() const { return left + width; }
    template <typename T> inline constexpr T rectangle<T>::bottom() const { return top + height; }
    template <typename T> inline constexpr bool rectangle<T>::contains(T x, T y) const { return x >= left && x <= right() && y >= top && y <= bottom(); }
    template <typename T> inline constexpr bool rectangle<T>::contains(const rectangle<T>& other) const { return contains(other.left, other.top) and contains(other.right(), other.bottom()); }
    template <typename T> inline constexpr bool rectangle<T>::intersects(const rectangle<T>& other) const { return !(other.left > right() || other.right() < left || other.top > bottom() || other.bottom() < top); }
    template <typename T> inline constexpr std::optional<rectangle<T>> rectangle<T>::intersection(const rectangle<T>& other) const
    {
        const T newLeft = std::max(left, other.left);
        const T newTop = std::max(top, other.top);
        const T newRight = std::min(right(), other.right());
        const T newBottom = std::min(bottom(), other.bottom());

        if (newLeft >= newRight or newTop >= newBottom) {
            return std::nullopt;
        }

        return rectangle<T> {newLeft, newTop, newRight - newLeft, newBottom - newTop};
    }

    template <typename T> inline constexpr bool rectangle<T>::operator==(const rectangle<T>& other) const { return left == other.left and top == other.top and width == other.width and height == other.height; }
    template <typename T> inline constexpr bool rectangle<T>::operator!=(const rectangle<T>& other) const { return not(*this == other); }
} // namespace p5cpp
