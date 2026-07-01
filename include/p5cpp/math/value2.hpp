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

        value2 normalized() const;
        value2 limited(T maxLength) const;
        value2 fixedLength(T newLength) const;
        value2 rotated(float radians) const;
        value2 rotatedAround(float radians, const value2& center) const;

        T length() const;

        constexpr value2 perpendicular() const;
        constexpr value2 reflected(const value2& normal) const;
        constexpr value2 projected(const value2& onto) const;
        constexpr value2 rejected(const value2& onto) const;

        constexpr T lengthSquared() const;
        constexpr T dot(const value2& other) const;
        constexpr T cross(const value2& other) const;

        constexpr value2 lerp(const value2& other, T t) const;
        constexpr value2 lerp(const value2& other, const value2& t) const;

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
        constexpr bool operator!=(const value2& other) const;

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

    template <typename T> inline value2<T> value2<T>::normalized() const { return fixedLength(static_cast<T>(1)); }
    template <typename T> inline value2<T> value2<T>::limited(T maxLength) const { return lengthSquared() > maxLength * maxLength ? fixedLength(maxLength) : *this; }
    template <typename T> inline value2<T> value2<T>::fixedLength(T newLength) const
    {
        const double len = static_cast<double>(lengthSquared());
        if (len == 0.0) {
            return {static_cast<T>(0), static_cast<T>(0)};
        }

        const double scale = static_cast<double>(newLength) / std::sqrt(len);
        return {static_cast<T>(x * scale), static_cast<T>(y * scale)};
    }

    template <typename T> inline value2<T> value2<T>::rotated(float radians) const
    {
        const float cosTheta = std::cos(radians);
        const float sinTheta = std::sin(radians);
        return {static_cast<T>(x * cosTheta - y * sinTheta), static_cast<T>(x * sinTheta + y * cosTheta)};
    }

    template <typename T> inline value2<T> value2<T>::rotatedAround(float radians, const value2& center) const
    {
        const float cosTheta = std::cos(radians);
        const float sinTheta = std::sin(radians);
        const T translatedX = x - center.x;
        const T translatedY = y - center.y;
        return {
            static_cast<T>(translatedX * cosTheta - translatedY * sinTheta + center.x),
            static_cast<T>(translatedX * sinTheta + translatedY * cosTheta + center.y),
        };
    }

    template <typename T> inline T value2<T>::length() const { return static_cast<T>(std::sqrt(static_cast<double>(lengthSquared()))); }

    template <typename T> inline constexpr value2<T> value2<T>::perpendicular() const { return {-y, x}; }
    template <typename T> inline constexpr value2<T> value2<T>::reflected(const value2& normal) const { return *this - normal * (static_cast<T>(2) * dot(normal)); }
    template <typename T> inline constexpr value2<T> value2<T>::projected(const value2& onto) const { return onto * (dot(onto) / onto.lengthSquared()); }
    template <typename T> inline constexpr value2<T> value2<T>::rejected(const value2& onto) const { return *this - projected(onto); }

    template <typename T> inline constexpr T value2<T>::lengthSquared() const { return x * x + y * y; }
    template <typename T> inline constexpr T value2<T>::dot(const value2& other) const { return x * other.x + y * other.y; }
    template <typename T> inline constexpr T value2<T>::cross(const value2& other) const { return x * other.y - y * other.x; }
    template <typename T> inline constexpr value2<T> value2<T>::lerp(const value2& other, T t) const { return {std::lerp(x, other.x, t), std::lerp(y, other.y, t)}; }
    template <typename T> inline constexpr value2<T> value2<T>::lerp(const value2& other, const value2& t) const { return {std::lerp(x, other.x, t.x), std::lerp(y, other.y, t.y)}; }

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
    template <typename T> inline constexpr bool value2<T>::operator!=(const value2& other) const { return not(*this == other); }

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
