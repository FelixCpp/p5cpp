#pragma once

#include <cmath>
#include <cstdint>

#include <p5cpp/math/constants.hpp>
#include <p5cpp/math/random.hpp>

namespace p5cpp
{
    template <typename T>
    struct value2
    {
        constexpr value2();
        constexpr value2(T x, T y);

        template <typename U>
        constexpr explicit value2(const value2<U>& other);

        constexpr value2 operator+(const value2& other) const;
        constexpr value2 operator-(const value2& other) const;
        constexpr value2 operator*(const value2& other) const;
        constexpr value2 operator/(const value2& other) const;

        constexpr value2 operator+(T scalar) const;
        constexpr value2 operator-(T scalar) const;
        constexpr value2 operator*(T scalar) const;
        constexpr value2 operator/(T scalar) const;

        constexpr value2 operator-() const;

        value2& operator+=(const value2& other);
        value2& operator-=(const value2& other);
        value2& operator*=(const value2& other);
        value2& operator/=(const value2& other);

        value2& operator+=(T scalar);
        value2& operator-=(T scalar);
        value2& operator*=(T scalar);
        value2& operator/=(T scalar);

        constexpr bool operator==(const value2& other) const;

        static const value2 zero;
        static const value2 one;
        static const value2 up;
        static const value2 down;
        static const value2 left;
        static const value2 right;

        static value2 fromAngle(float radians);
        static value2 randomUnit();

        T x, y;
    };

    typedef value2<float> float2;
    typedef value2<int32_t> int2;
    typedef value2<uint32_t> uint2;

    template <typename T> value2<T> normalized(const value2<T>& v);
    template <typename T> value2<T> limited(const value2<T>& v, T maxLength);
    template <typename T> value2<T> fixedLength(const value2<T>& v, T newLength);
    template <typename T> value2<T> rotated(const value2<T>& v, float radians);
    template <typename T> value2<T> rotatedAround(const value2<T>& v, float radians, const value2<T>& center);

    template <typename T> T length(const value2<T>& v);

    template <typename T> constexpr value2<T> perpendicular(const value2<T>& v);
    template <typename T> constexpr value2<T> reflected(const value2<T>& v, const value2<T>& normal);
    template <typename T> constexpr value2<T> projected(const value2<T>& v, const value2<T>& onto);
    template <typename T> constexpr value2<T> rejected(const value2<T>& v, const value2<T>& onto);

    template <typename T> constexpr T lengthSquared(const value2<T>& v);
    template <typename T> constexpr T dot(const value2<T>& a, const value2<T>& b);
    template <typename T> constexpr T cross(const value2<T>& a, const value2<T>& b);

    template <typename T> constexpr value2<T> lerp(const value2<T>& a, const value2<T>& b, T t);
    template <typename T> constexpr value2<T> lerp(const value2<T>& a, const value2<T>& b, const value2<T>& t);
} // namespace p5cpp

namespace p5cpp
{
    template <typename T>
    inline constexpr value2<T>::value2()
        : x(T {}), y(T {})
    {
    }

    template <typename T>
    inline constexpr value2<T>::value2(T x, T y)
        : x(x), y(y)
    {
    }

    template <typename T>
    template <typename U>
    inline constexpr value2<T>::value2(const value2<U>& other)
        : x(static_cast<T>(other.x)), y(static_cast<T>(other.y))
    {
    }

    template <typename T> inline value2<T> normalized(const value2<T>& v) { return fixedLength(v, static_cast<T>(1)); }
    template <typename T> inline value2<T> limited(const value2<T>& v, T maxLength) { return lengthSquared(v) > maxLength * maxLength ? fixedLength(v, maxLength) : v; }
    template <typename T> inline value2<T> fixedLength(const value2<T>& v, T newLength)
    {
        const double len = static_cast<double>(lengthSquared(v));
        if (len == 0.0) {
            return {static_cast<T>(0), static_cast<T>(0)};
        }

        const double scale = static_cast<double>(newLength) / std::sqrt(len);
        return {static_cast<T>(v.x * scale), static_cast<T>(v.y * scale)};
    }

    template <typename T> inline value2<T> rotated(const value2<T>& v, float radians)
    {
        const float cosTheta = std::cos(radians);
        const float sinTheta = std::sin(radians);
        return {static_cast<T>(v.x * cosTheta - v.y * sinTheta), static_cast<T>(v.x * sinTheta + v.y * cosTheta)};
    }

    template <typename T> inline value2<T> rotatedAround(const value2<T>& v, float radians, const value2<T>& center)
    {
        const float cosTheta = std::cos(radians);
        const float sinTheta = std::sin(radians);
        const T translatedX = v.x - center.x;
        const T translatedY = v.y - center.y;
        return {
            static_cast<T>(translatedX * cosTheta - translatedY * sinTheta + center.x),
            static_cast<T>(translatedX * sinTheta + translatedY * cosTheta + center.y),
        };
    }

    template <typename T> inline T length(const value2<T>& v) { return static_cast<T>(std::sqrt(static_cast<double>(lengthSquared(v)))); }

    template <typename T> inline constexpr value2<T> perpendicular(const value2<T>& v) { return {-v.y, v.x}; }
    template <typename T> inline constexpr value2<T> reflected(const value2<T>& v, const value2<T>& normal) { return v - normal * (static_cast<T>(2) * dot(v, normal)); }
    template <typename T> inline constexpr value2<T> projected(const value2<T>& v, const value2<T>& onto) { return onto * (dot(v, onto) / lengthSquared(onto)); }
    template <typename T> inline constexpr value2<T> rejected(const value2<T>& v, const value2<T>& onto) { return v - projected(v, onto); }

    template <typename T> inline constexpr T lengthSquared(const value2<T>& v) { return v.x * v.x + v.y * v.y; }
    template <typename T> inline constexpr T dot(const value2<T>& a, const value2<T>& b) { return a.x * b.x + a.y * b.y; }
    template <typename T> inline constexpr T cross(const value2<T>& a, const value2<T>& b) { return a.x * b.y - a.y * b.x; }
    template <typename T> inline constexpr value2<T> lerp(const value2<T>& a, const value2<T>& b, T t) { return {std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t)}; }
    template <typename T> inline constexpr value2<T> lerp(const value2<T>& a, const value2<T>& b, const value2<T>& t) { return {std::lerp(a.x, b.x, t.x), std::lerp(a.y, b.y, t.y)}; }

    template <typename T> inline constexpr value2<T> value2<T>::operator+(const value2& other) const { return {x + other.x, y + other.y}; }
    template <typename T> inline constexpr value2<T> value2<T>::operator-(const value2& other) const { return {x - other.x, y - other.y}; }
    template <typename T> inline constexpr value2<T> value2<T>::operator*(const value2& other) const { return {x * other.x, y * other.y}; }
    template <typename T> inline constexpr value2<T> value2<T>::operator/(const value2& other) const { return {x / other.x, y / other.y}; }

    template <typename T> inline constexpr value2<T> value2<T>::operator+(T scalar) const { return {x + scalar, y + scalar}; }
    template <typename T> inline constexpr value2<T> value2<T>::operator-(T scalar) const { return {x - scalar, y - scalar}; }
    template <typename T> inline constexpr value2<T> value2<T>::operator*(T scalar) const { return {x * scalar, y * scalar}; }
    template <typename T> inline constexpr value2<T> value2<T>::operator/(T scalar) const { return {x / scalar, y / scalar}; }

    template <typename T> inline constexpr value2<T> value2<T>::operator-() const { return {-x, -y}; }

    template <typename T> inline value2<T>& value2<T>::operator+=(const value2& other) { return *this = *this + other; }
    template <typename T> inline value2<T>& value2<T>::operator-=(const value2& other) { return *this = *this - other; }
    template <typename T> inline value2<T>& value2<T>::operator*=(const value2& other) { return *this = *this * other; }
    template <typename T> inline value2<T>& value2<T>::operator/=(const value2& other) { return *this = *this / other; }
    template <typename T> inline value2<T>& value2<T>::operator+=(T scalar) { return *this = *this + scalar; }
    template <typename T> inline value2<T>& value2<T>::operator-=(T scalar) { return *this = *this - scalar; }
    template <typename T> inline value2<T>& value2<T>::operator*=(T scalar) { return *this = *this * scalar; }
    template <typename T> inline value2<T>& value2<T>::operator/=(T scalar) { return *this = *this / scalar; }

    template <typename T> inline constexpr bool value2<T>::operator==(const value2& other) const { return x == other.x and y == other.y; }

    template <typename T> inline value2<T> value2<T>::fromAngle(float radians) { return {static_cast<T>(std::cos(radians)), static_cast<T>(std::sin(radians))}; }
    template <typename T> inline value2<T> value2<T>::randomUnit() { return fromAngle(randomFloat(0.0f, TWO_PI)); }

    template <typename T> inline const value2<T> value2<T>::zero = {static_cast<T>(0), static_cast<T>(0)};
    template <typename T> inline const value2<T> value2<T>::one = {static_cast<T>(1), static_cast<T>(1)};
    template <typename T> inline const value2<T> value2<T>::up = {static_cast<T>(0), static_cast<T>(1)};
    template <typename T> inline const value2<T> value2<T>::down = {static_cast<T>(0), static_cast<T>(-1)};
    template <typename T> inline const value2<T> value2<T>::left = {static_cast<T>(-1), static_cast<T>(0)};
    template <typename T> inline const value2<T> value2<T>::right = {static_cast<T>(1), static_cast<T>(0)};

    template <typename T> inline value2<T> operator+(T scalar, const value2<T>& vec) { return vec + scalar; }
    template <typename T> inline value2<T> operator-(T scalar, const value2<T>& vec) { return {scalar - vec.x, scalar - vec.y}; }
    template <typename T> inline value2<T> operator*(T scalar, const value2<T>& vec) { return vec * scalar; }
    template <typename T> inline value2<T> operator/(T scalar, const value2<T>& vec) { return {scalar / vec.x, scalar / vec.y}; }
} // namespace p5cpp
