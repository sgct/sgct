/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __MATH_H__
#define __MATH_H__

namespace sgct {

struct ivec2 {
    int x, y;
};

struct ivec3 {
    int x, y, z;
};

struct ivec4 {
    int x, y, z, w;
};

struct vec2 {
    float x, y;
};

struct vec3 {
    float x, y, z;
};

struct vec4 {
    float x, y, z, w;
};

struct quat {
    float x, y, z, w;
};

// Column-major order matrix
struct mat4 {
    mat4() = default;
    mat4(float v) {
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
    float values[16];
};

mat4 operator*(const mat4& m1, const mat4& m2);
vec4 operator*(const mat4& m, const vec4& v);
vec3 operator*(const quat& q, const vec3& v);

} // namespace sgct

#endif // __MATH_H_HH
