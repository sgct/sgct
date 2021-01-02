/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2021                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/math.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace sgct {

template <typename From, typename To>
To fromGLM(From v) {
    To r;
    std::memcpy(&r, glm::value_ptr(v), sizeof(To));
    return r;
}

mat4 operator*(const mat4& m1, const mat4& m2) {
    return fromGLM<glm::mat4, mat4>(
        glm::make_mat4(m1.values) * glm::make_mat4(m2.values)
    );
}

vec4 operator*(const mat4& m, const vec4& v) {
    return fromGLM<glm::vec4, vec4>(glm::make_mat4(m.values) * glm::make_vec4(&v.x));
}

vec3 operator*(const quat& q, const vec3& v) {
    return fromGLM<glm::vec3, vec3>(glm::make_quat(&q.x) * glm::make_vec3(&v.x));
}

} // namespace
