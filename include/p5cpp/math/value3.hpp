#pragma once

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

        T x, y, z;
    };

    typedef value3<float> float3;
    typedef value3<int32_t> int3;
    typedef value3<uint32_t> uint3;
} // namespace p5cpp

namespace p5cpp
{
    template <typename T> constexpr value3<T>::value3() : x(T {}), y(T {}), z(T {}) {}
    template <typename T> constexpr value3<T>::value3(T x, T y, T z) : x(x), y(y), z(z) {}
    template <typename T> constexpr value3<T>::value3(T scalar) : x(scalar), y(scalar), z(scalar) {}
    template <typename T> template <typename U> constexpr value3<T>::value3(const value3<U>& other) : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)), z(static_cast<T>(other.z)) {}
} // namespace p5cpp
