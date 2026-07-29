#pragma once

#include <cmath>
#include <cstdint>

namespace p5cpp
{
    template <typename T>
    struct value3
    {
        constexpr value3();
        constexpr value3(T x, T y, T z);
        constexpr explicit value3(T scalar);

        template <typename U>
        constexpr value3(const value3<U>& other);

        static const value3 zero;
        static const value3 one;

        T x, y, z;
    };

    typedef value3<float> float3;
    typedef value3<int32_t> int3;
    typedef value3<uint32_t> uint3;

    template <typename T> value3<T> normalized(const value3<T>& v);
    template <typename T> value3<T> limited(const value3<T>& v, T maxLength);
    template <typename T> value3<T> fixedLength(const value3<T>& v, T newLength);

    template <typename T> T length(const value3<T>& v);

    template <typename T> constexpr value3<T> reflected(const value3<T>& v, const value3<T>& normal);
    template <typename T> constexpr value3<T> projected(const value3<T>& v, const value3<T>& onto);
    template <typename T> constexpr value3<T> rejected(const value3<T>& v, const value3<T>& onto);

    template <typename T> constexpr T lengthSquared(const value3<T>& v);
    template <typename T> constexpr T dot(const value3<T>& a, const value3<T>& b);
    template <typename T> constexpr value3<T> cross(const value3<T>& a, const value3<T>& b);

    template <typename T> constexpr value3<T> lerp(const value3<T>& a, const value3<T>& b, T t);
    template <typename T> constexpr value3<T> lerp(const value3<T>& a, const value3<T>& b, const value3<T>& t);

    template <typename T> constexpr value3<T> operator+(const value3<T>& a, const value3<T>& b);
    template <typename T> constexpr value3<T> operator-(const value3<T>& a, const value3<T>& b);
    template <typename T> constexpr value3<T> operator*(const value3<T>& a, const value3<T>& b);
    template <typename T> constexpr value3<T> operator/(const value3<T>& a, const value3<T>& b);

    template <typename T> constexpr value3<T> operator+(const value3<T>& v, T scalar);
    template <typename T> constexpr value3<T> operator-(const value3<T>& v, T scalar);
    template <typename T> constexpr value3<T> operator*(const value3<T>& v, T scalar);
    template <typename T> constexpr value3<T> operator/(const value3<T>& v, T scalar);

    template <typename T> constexpr value3<T> operator-(const value3<T>& v);

    template <typename T> value3<T>& operator+=(value3<T>& a, const value3<T>& b);
    template <typename T> value3<T>& operator-=(value3<T>& a, const value3<T>& b);
    template <typename T> value3<T>& operator*=(value3<T>& a, const value3<T>& b);
    template <typename T> value3<T>& operator/=(value3<T>& a, const value3<T>& b);

    template <typename T> value3<T>& operator+=(value3<T>& v, T scalar);
    template <typename T> value3<T>& operator-=(value3<T>& v, T scalar);
    template <typename T> value3<T>& operator*=(value3<T>& v, T scalar);
    template <typename T> value3<T>& operator/=(value3<T>& v, T scalar);

    template <typename T> constexpr bool operator==(const value3<T>& a, const value3<T>& b);
    template <typename T> constexpr bool operator!=(const value3<T>& a, const value3<T>& b);

    template <typename T> value3<T> operator+(T scalar, const value3<T>& v);
    template <typename T> value3<T> operator-(T scalar, const value3<T>& v);
    template <typename T> value3<T> operator*(T scalar, const value3<T>& v);
    template <typename T> value3<T> operator/(T scalar, const value3<T>& v);
} // namespace p5cpp

namespace p5cpp
{
    template <typename T> inline constexpr value3<T>::value3() : x(T {}), y(T {}), z(T {}) {}
    template <typename T> inline constexpr value3<T>::value3(T x, T y, T z) : x(x), y(y), z(z) {}
    template <typename T> inline constexpr value3<T>::value3(T scalar) : x(scalar), y(scalar), z(scalar) {}
    template <typename T> template <typename U> inline constexpr value3<T>::value3(const value3<U>& other) : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)), z(static_cast<T>(other.z)) {}

    template <typename T> inline const value3<T> value3<T>::zero = {static_cast<T>(0), static_cast<T>(0), static_cast<T>(0)};
    template <typename T> inline const value3<T> value3<T>::one = {static_cast<T>(1), static_cast<T>(1), static_cast<T>(1)};

    template <typename T> inline value3<T> normalized(const value3<T>& v) { return fixedLength(v, static_cast<T>(1)); }
    template <typename T> inline value3<T> limited(const value3<T>& v, T maxLength) { return lengthSquared(v) > maxLength * maxLength ? fixedLength(v, maxLength) : v; }
    template <typename T> inline value3<T> fixedLength(const value3<T>& v, T newLength)
    {
        const double len = static_cast<double>(lengthSquared(v));
        if (len == 0.0) {
            return {static_cast<T>(0), static_cast<T>(0), static_cast<T>(0)};
        }

        const double scale = static_cast<double>(newLength) / std::sqrt(len);
        return {static_cast<T>(v.x * scale), static_cast<T>(v.y * scale), static_cast<T>(v.z * scale)};
    }

    template <typename T> inline T length(const value3<T>& v) { return static_cast<T>(std::sqrt(static_cast<double>(lengthSquared(v)))); }

    template <typename T> inline constexpr value3<T> reflected(const value3<T>& v, const value3<T>& normal) { return v - normal * (static_cast<T>(2) * dot(v, normal)); }
    template <typename T> inline constexpr value3<T> projected(const value3<T>& v, const value3<T>& onto) { return onto * (dot(v, onto) / lengthSquared(onto)); }
    template <typename T> inline constexpr value3<T> rejected(const value3<T>& v, const value3<T>& onto) { return v - projected(v, onto); }

    template <typename T> inline constexpr T lengthSquared(const value3<T>& v) { return v.x * v.x + v.y * v.y + v.z * v.z; }
    template <typename T> inline constexpr T dot(const value3<T>& a, const value3<T>& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    template <typename T> inline constexpr value3<T> cross(const value3<T>& a, const value3<T>& b)
    {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x,
        };
    }

    template <typename T> inline constexpr value3<T> lerp(const value3<T>& a, const value3<T>& b, T t) { return {std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t), std::lerp(a.z, b.z, t)}; }
    template <typename T> inline constexpr value3<T> lerp(const value3<T>& a, const value3<T>& b, const value3<T>& t) { return {std::lerp(a.x, b.x, t.x), std::lerp(a.y, b.y, t.y), std::lerp(a.z, b.z, t.z)}; }

    template <typename T> inline constexpr value3<T> operator+(const value3<T>& a, const value3<T>& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
    template <typename T> inline constexpr value3<T> operator-(const value3<T>& a, const value3<T>& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
    template <typename T> inline constexpr value3<T> operator*(const value3<T>& a, const value3<T>& b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }
    template <typename T> inline constexpr value3<T> operator/(const value3<T>& a, const value3<T>& b) { return {a.x / b.x, a.y / b.y, a.z / b.z}; }

    template <typename T> inline constexpr value3<T> operator+(const value3<T>& v, T scalar) { return {v.x + scalar, v.y + scalar, v.z + scalar}; }
    template <typename T> inline constexpr value3<T> operator-(const value3<T>& v, T scalar) { return {v.x - scalar, v.y - scalar, v.z - scalar}; }
    template <typename T> inline constexpr value3<T> operator*(const value3<T>& v, T scalar) { return {v.x * scalar, v.y * scalar, v.z * scalar}; }
    template <typename T> inline constexpr value3<T> operator/(const value3<T>& v, T scalar) { return {v.x / scalar, v.y / scalar, v.z / scalar}; }

    template <typename T> inline constexpr value3<T> operator-(const value3<T>& v) { return {-v.x, -v.y, -v.z}; }

    template <typename T> inline value3<T>& operator+=(value3<T>& a, const value3<T>& b) { return a = a + b; }
    template <typename T> inline value3<T>& operator-=(value3<T>& a, const value3<T>& b) { return a = a - b; }
    template <typename T> inline value3<T>& operator*=(value3<T>& a, const value3<T>& b) { return a = a * b; }
    template <typename T> inline value3<T>& operator/=(value3<T>& a, const value3<T>& b) { return a = a / b; }
    template <typename T> inline value3<T>& operator+=(value3<T>& v, T scalar) { return v = v + scalar; }
    template <typename T> inline value3<T>& operator-=(value3<T>& v, T scalar) { return v = v - scalar; }
    template <typename T> inline value3<T>& operator*=(value3<T>& v, T scalar) { return v = v * scalar; }
    template <typename T> inline value3<T>& operator/=(value3<T>& v, T scalar) { return v = v / scalar; }

    template <typename T> inline constexpr bool operator==(const value3<T>& a, const value3<T>& b) { return a.x == b.x and a.y == b.y and a.z == b.z; }
    template <typename T> inline constexpr bool operator!=(const value3<T>& a, const value3<T>& b) { return not(a == b); }

    template <typename T> inline value3<T> operator+(T scalar, const value3<T>& v) { return v + scalar; }
    template <typename T> inline value3<T> operator-(T scalar, const value3<T>& v) { return {scalar - v.x, scalar - v.y, scalar - v.z}; }
    template <typename T> inline value3<T> operator*(T scalar, const value3<T>& v) { return v * scalar; }
    template <typename T> inline value3<T> operator/(T scalar, const value3<T>& v) { return {scalar / v.x, scalar / v.y, scalar / v.z}; }
} // namespace p5cpp
