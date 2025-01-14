/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/math.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace sgct {

mat4 operator*(const mat4& m1, const mat4& m2) {
    const glm::mat4 r =
        glm::make_mat4(m1.values.data()) * glm::make_mat4(m2.values.data());
    mat4 res;
    std::memcpy(&res, glm::value_ptr(r), sizeof(float[16]));
    return res;
}

vec4 operator*(const mat4& m, const vec4& v) {
    const glm::vec4 r = glm::make_mat4(m.values.data()) * glm::make_vec4(&v.x);
    return vec4(r.x, r.y, r.z, r.w);
}

vec3 operator*(const quat& q, const vec3& v) {
    const glm::vec3 r = glm::make_quat(&q.x) * glm::make_vec3(&v.x);
    return vec3(r.x, r.y, r.z);
}

} // namespace sgct
