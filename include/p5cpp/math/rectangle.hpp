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

    template <typename T> constexpr T right(const rectangle<T>& r);
    template <typename T> constexpr T bottom(const rectangle<T>& r);

    template <typename T> constexpr bool contains(const rectangle<T>& r, T x, T y);
    template <typename T> constexpr bool contains(const rectangle<T>& r, const rectangle<T>& other);
    template <typename T> constexpr bool intersects(const rectangle<T>& r, const rectangle<T>& other);
    template <typename T> constexpr std::optional<rectangle<T>> intersection(const rectangle<T>& r, const rectangle<T>& other);

    template <typename T> constexpr bool operator==(const rectangle<T>& a, const rectangle<T>& b);
    template <typename T> constexpr bool operator!=(const rectangle<T>& a, const rectangle<T>& b);
} // namespace p5cpp

namespace p5cpp
{
    template <typename T> inline constexpr rectangle<T>::rectangle() : left(T {}), top(T {}), width(T {}), height(T {}) {}
    template <typename T> inline constexpr rectangle<T>::rectangle(T left, T top, T width, T height) : left(left), top(top), width(width), height(height) {}
    template <typename T> template <typename U> inline constexpr rectangle<T>::rectangle(const rectangle<U>& other) : left(static_cast<T>(other.left)), top(static_cast<T>(other.top)), width(static_cast<T>(other.width)), height(static_cast<T>(other.height)) {}

    template <typename T> inline constexpr T right(const rectangle<T>& r) { return r.left + r.width; }
    template <typename T> inline constexpr T bottom(const rectangle<T>& r) { return r.top + r.height; }
    template <typename T> inline constexpr bool contains(const rectangle<T>& r, T x, T y) { return x >= r.left && x <= right(r) && y >= r.top && y <= bottom(r); }
    template <typename T> inline constexpr bool contains(const rectangle<T>& r, const rectangle<T>& other) { return contains(r, other.left, other.top) and contains(r, right(other), bottom(other)); }
    template <typename T> inline constexpr bool intersects(const rectangle<T>& r, const rectangle<T>& other) { return !(other.left > right(r) || right(other) < r.left || other.top > bottom(r) || bottom(other) < r.top); }
    template <typename T> inline constexpr std::optional<rectangle<T>> intersection(const rectangle<T>& r, const rectangle<T>& other)
    {
        const T newLeft = std::max(r.left, other.left);
        const T newTop = std::max(r.top, other.top);
        const T newRight = std::min(right(r), right(other));
        const T newBottom = std::min(bottom(r), bottom(other));

        if (newLeft >= newRight or newTop >= newBottom) {
            return std::nullopt;
        }

        return rectangle<T> {newLeft, newTop, newRight - newLeft, newBottom - newTop};
    }

    template <typename T> inline constexpr bool operator==(const rectangle<T>& a, const rectangle<T>& b) { return a.left == b.left and a.top == b.top and a.width == b.width and a.height == b.height; }
    template <typename T> inline constexpr bool operator!=(const rectangle<T>& a, const rectangle<T>& b) { return not(a == b); }
} // namespace p5cpp
