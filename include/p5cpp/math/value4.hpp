#pragma once

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

        T x, y, z, w;
    };

    typedef value4<float> float4;
    typedef value4<int32_t> int4;
    typedef value4<uint32_t> uint4;
} // namespace p5cpp

namespace p5cpp
{
    template <typename T> constexpr value4<T>::value4() : x(T {}), y(T {}), z(T {}), w(T {}) {}
    template <typename T> constexpr value4<T>::value4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    template <typename T> constexpr value4<T>::value4(T scalar) : x(scalar), y(scalar), z(scalar), w(scalar) {}
    template <typename T> template <typename U> constexpr value4<T>::value4(const value4<U>& other) : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)), z(static_cast<T>(other.z)), w(static_cast<T>(other.w)) {}
} // namespace p5cpp
