#pragma once

#include <cmath>
#include <cstdint>

namespace p5cpp
{
    template <typename T>
    struct value4
    {
        constexpr value4();
        constexpr value4(T x, T y, T z, T w);
        constexpr explicit value4(T scalar);

        template <typename U>
        constexpr value4(const value4<U>& other);

        static const value4 zero;
        static const value4 one;

        T x, y, z, w;
    };

    typedef value4<float> float4;
    typedef value4<int32_t> int4;
    typedef value4<uint32_t> uint4;

    template <typename T> value4<T> normalized(const value4<T>& v);
    template <typename T> value4<T> limited(const value4<T>& v, T maxLength);
    template <typename T> value4<T> fixedLength(const value4<T>& v, T newLength);

    template <typename T> T length(const value4<T>& v);

    template <typename T> constexpr value4<T> reflected(const value4<T>& v, const value4<T>& normal);
    template <typename T> constexpr value4<T> projected(const value4<T>& v, const value4<T>& onto);
    template <typename T> constexpr value4<T> rejected(const value4<T>& v, const value4<T>& onto);

    template <typename T> constexpr T lengthSquared(const value4<T>& v);
    template <typename T> constexpr T dot(const value4<T>& a, const value4<T>& b);

    template <typename T> constexpr value4<T> lerp(const value4<T>& a, const value4<T>& b, T t);
    template <typename T> constexpr value4<T> lerp(const value4<T>& a, const value4<T>& b, const value4<T>& t);

    template <typename T> constexpr value4<T> operator+(const value4<T>& a, const value4<T>& b);
    template <typename T> constexpr value4<T> operator-(const value4<T>& a, const value4<T>& b);
    template <typename T> constexpr value4<T> operator*(const value4<T>& a, const value4<T>& b);
    template <typename T> constexpr value4<T> operator/(const value4<T>& a, const value4<T>& b);

    template <typename T> constexpr value4<T> operator+(const value4<T>& v, T scalar);
    template <typename T> constexpr value4<T> operator-(const value4<T>& v, T scalar);
    template <typename T> constexpr value4<T> operator*(const value4<T>& v, T scalar);
    template <typename T> constexpr value4<T> operator/(const value4<T>& v, T scalar);

    template <typename T> constexpr value4<T> operator-(const value4<T>& v);

    template <typename T> value4<T>& operator+=(value4<T>& a, const value4<T>& b);
    template <typename T> value4<T>& operator-=(value4<T>& a, const value4<T>& b);
    template <typename T> value4<T>& operator*=(value4<T>& a, const value4<T>& b);
    template <typename T> value4<T>& operator/=(value4<T>& a, const value4<T>& b);

    template <typename T> value4<T>& operator+=(value4<T>& v, T scalar);
    template <typename T> value4<T>& operator-=(value4<T>& v, T scalar);
    template <typename T> value4<T>& operator*=(value4<T>& v, T scalar);
    template <typename T> value4<T>& operator/=(value4<T>& v, T scalar);

    template <typename T> constexpr bool operator==(const value4<T>& a, const value4<T>& b);
    template <typename T> constexpr bool operator!=(const value4<T>& a, const value4<T>& b);

    template <typename T> value4<T> operator+(T scalar, const value4<T>& v);
    template <typename T> value4<T> operator-(T scalar, const value4<T>& v);
    template <typename T> value4<T> operator*(T scalar, const value4<T>& v);
    template <typename T> value4<T> operator/(T scalar, const value4<T>& v);
} // namespace p5cpp

namespace p5cpp
{
    template <typename T> inline constexpr value4<T>::value4() : x(T {}), y(T {}), z(T {}), w(T {}) {}
    template <typename T> inline constexpr value4<T>::value4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    template <typename T> inline constexpr value4<T>::value4(T scalar) : x(scalar), y(scalar), z(scalar), w(scalar) {}
    template <typename T> template <typename U> inline constexpr value4<T>::value4(const value4<U>& other) : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)), z(static_cast<T>(other.z)), w(static_cast<T>(other.w)) {}

    template <typename T> inline const value4<T> value4<T>::zero = {static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0)};
    template <typename T> inline const value4<T> value4<T>::one = {static_cast<T>(1), static_cast<T>(1), static_cast<T>(1), static_cast<T>(1)};

    template <typename T> inline value4<T> normalized(const value4<T>& v) { return fixedLength(v, static_cast<T>(1)); }
    template <typename T> inline value4<T> limited(const value4<T>& v, T maxLength) { return lengthSquared(v) > maxLength * maxLength ? fixedLength(v, maxLength) : v; }
    template <typename T> inline value4<T> fixedLength(const value4<T>& v, T newLength)
    {
        const double len = static_cast<double>(lengthSquared(v));
        if (len == 0.0) {
            return {static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0)};
        }

        const double scale = static_cast<double>(newLength) / std::sqrt(len);
        return {static_cast<T>(v.x * scale), static_cast<T>(v.y * scale), static_cast<T>(v.z * scale), static_cast<T>(v.w * scale)};
    }

    template <typename T> inline T length(const value4<T>& v) { return static_cast<T>(std::sqrt(static_cast<double>(lengthSquared(v)))); }

    template <typename T> inline constexpr value4<T> reflected(const value4<T>& v, const value4<T>& normal) { return v - normal * (static_cast<T>(2) * dot(v, normal)); }
    template <typename T> inline constexpr value4<T> projected(const value4<T>& v, const value4<T>& onto) { return onto * (dot(v, onto) / lengthSquared(onto)); }
    template <typename T> inline constexpr value4<T> rejected(const value4<T>& v, const value4<T>& onto) { return v - projected(v, onto); }

    template <typename T> inline constexpr T lengthSquared(const value4<T>& v) { return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w; }
    template <typename T> inline constexpr T dot(const value4<T>& a, const value4<T>& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

    template <typename T> inline constexpr value4<T> lerp(const value4<T>& a, const value4<T>& b, T t) { return {std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t), std::lerp(a.z, b.z, t), std::lerp(a.w, b.w, t)}; }
    template <typename T> inline constexpr value4<T> lerp(const value4<T>& a, const value4<T>& b, const value4<T>& t) { return {std::lerp(a.x, b.x, t.x), std::lerp(a.y, b.y, t.y), std::lerp(a.z, b.z, t.z), std::lerp(a.w, b.w, t.w)}; }

    template <typename T> inline constexpr value4<T> operator+(const value4<T>& a, const value4<T>& b) { return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }
    template <typename T> inline constexpr value4<T> operator-(const value4<T>& a, const value4<T>& b) { return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }
    template <typename T> inline constexpr value4<T> operator*(const value4<T>& a, const value4<T>& b) { return {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w}; }
    template <typename T> inline constexpr value4<T> operator/(const value4<T>& a, const value4<T>& b) { return {a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w}; }

    template <typename T> inline constexpr value4<T> operator+(const value4<T>& v, T scalar) { return {v.x + scalar, v.y + scalar, v.z + scalar, v.w + scalar}; }
    template <typename T> inline constexpr value4<T> operator-(const value4<T>& v, T scalar) { return {v.x - scalar, v.y - scalar, v.z - scalar, v.w - scalar}; }
    template <typename T> inline constexpr value4<T> operator*(const value4<T>& v, T scalar) { return {v.x * scalar, v.y * scalar, v.z * scalar, v.w * scalar}; }
    template <typename T> inline constexpr value4<T> operator/(const value4<T>& v, T scalar) { return {v.x / scalar, v.y / scalar, v.z / scalar, v.w / scalar}; }

    template <typename T> inline constexpr value4<T> operator-(const value4<T>& v) { return {-v.x, -v.y, -v.z, -v.w}; }

    template <typename T> inline value4<T>& operator+=(value4<T>& a, const value4<T>& b) { return a = a + b; }
    template <typename T> inline value4<T>& operator-=(value4<T>& a, const value4<T>& b) { return a = a - b; }
    template <typename T> inline value4<T>& operator*=(value4<T>& a, const value4<T>& b) { return a = a * b; }
    template <typename T> inline value4<T>& operator/=(value4<T>& a, const value4<T>& b) { return a = a / b; }
    template <typename T> inline value4<T>& operator+=(value4<T>& v, T scalar) { return v = v + scalar; }
    template <typename T> inline value4<T>& operator-=(value4<T>& v, T scalar) { return v = v - scalar; }
    template <typename T> inline value4<T>& operator*=(value4<T>& v, T scalar) { return v = v * scalar; }
    template <typename T> inline value4<T>& operator/=(value4<T>& v, T scalar) { return v = v / scalar; }

    template <typename T> inline constexpr bool operator==(const value4<T>& a, const value4<T>& b) { return a.x == b.x and a.y == b.y and a.z == b.z and a.w == b.w; }
    template <typename T> inline constexpr bool operator!=(const value4<T>& a, const value4<T>& b) { return not(a == b); }

    template <typename T> inline value4<T> operator+(T scalar, const value4<T>& v) { return v + scalar; }
    template <typename T> inline value4<T> operator-(T scalar, const value4<T>& v) { return {scalar - v.x, scalar - v.y, scalar - v.z, scalar - v.w}; }
    template <typename T> inline value4<T> operator*(T scalar, const value4<T>& v) { return v * scalar; }
    template <typename T> inline value4<T> operator/(T scalar, const value4<T>& v) { return {scalar / v.x, scalar / v.y, scalar / v.z, scalar / v.w}; }
} // namespace p5cpp
