/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__MATH_H__
#define __SGCT__MATH_H__

#include <sgct/sgctexports.h>
#include <array>
#include <compare>

namespace sgct {

struct SGCT_EXPORT ivec2 {
    constexpr ivec2() = default;
    constexpr ivec2(int x_, int y_) : x(x_), y(y_) {}
    auto operator<=>(const ivec2&) const noexcept = default;

    int x = 0;
    int y = 0;
};

struct SGCT_EXPORT ivec3 {
    constexpr ivec3() = default;
    constexpr ivec3(int x_, int y_, int z_) : x(x_), y(y_), z(z_) {}
    auto operator<=>(const ivec3&) const noexcept = default;

    int x = 0;
    int y = 0;
    int z = 0;
};

struct SGCT_EXPORT ivec4 {
    constexpr ivec4() = default;
    constexpr ivec4(int x_, int y_, int z_, int w_) : x(x_), y(y_), z(z_), w(w_) {}
    auto operator<=>(const ivec4&) const noexcept = default;

    int x = 0;
    int y = 0;
    int z = 0;
    int w = 0;
};

struct SGCT_EXPORT vec2 {
    constexpr vec2() = default;
    constexpr vec2(float x_, float y_) : x(x_), y(y_) {}
    auto operator<=>(const vec2&) const noexcept = default;

    float x = 0.f;
    float y = 0.f;
};

struct SGCT_EXPORT vec3 {
    constexpr vec3() = default;
    constexpr vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    auto operator<=>(const vec3&) const noexcept = default;

    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
};

struct SGCT_EXPORT vec4 {
    constexpr vec4() = default;
    constexpr vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
    auto operator<=>(const vec4&) const noexcept = default;

    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
    float w = 0.f;
};

struct SGCT_EXPORT quat {
    constexpr quat() = default;
    constexpr quat(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
    auto operator<=>(const quat&) const noexcept = default;

    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
    float w = 0.f;
};

// Column-major order matrix
struct SGCT_EXPORT mat4 {
    constexpr mat4() = default;
    constexpr mat4(float v) {
        values[0 * 4 + 0] = v;
        values[0 * 4 + 1] = 0.f;
        values[0 * 4 + 2] = 0.f;
        values[0 * 4 + 3] = 0.f;
        values[1 * 4 + 0] = 0.f;
        values[1 * 4 + 1] = v;
        values[1 * 4 + 2] = 0.f;
        values[1 * 4 + 3] = 0.f;
        values[2 * 4 + 0] = 0.f;
        values[2 * 4 + 1] = 0.f;
        values[2 * 4 + 2] = v;
        values[2 * 4 + 3] = 0.f;
        values[3 * 4 + 0] = 0.f;
        values[3 * 4 + 1] = 0.f;
        values[3 * 4 + 2] = 0.f;
        values[3 * 4 + 3] = v;
    }
    auto operator<=>(const mat4&) const noexcept = default;

    std::array<float, 16> values;
};

SGCT_EXPORT mat4 operator*(const mat4& m1, const mat4& m2);
SGCT_EXPORT vec4 operator*(const mat4& m, const vec4& v);
SGCT_EXPORT vec3 operator*(const quat& q, const vec3& v);

} // namespace sgct

#endif // __SGCT__MATH_H__
