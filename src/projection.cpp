/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection.h>

#include <sgct/projection/projectionplane.h>
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable : 4127)
#endif // WIN32
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifdef WIN32
#pragma warning(pop)
#endif // WIN32

#include <cstring>

namespace sgct {

void Projection::calculateProjection(vec3 base, const ProjectionPlane& proj,
                                     float nearClip, float farClip, vec3 offset)
{
    const glm::vec3 b = glm::make_vec3(&base.x);
    const glm::vec3 o = glm::make_vec3(&offset.x);

    const glm::vec3 lowerLeft = glm::make_vec3(&proj.coordinateLowerLeft().x);
    const glm::vec3 upperLeft = glm::make_vec3(&proj.coordinateUpperLeft().x);
    const glm::vec3 upperRight = glm::make_vec3(&proj.coordinateUpperRight().x);

    // calculate viewplane's internal coordinate system bases
    const glm::vec3 planeX = glm::normalize(upperRight - upperLeft);
    const glm::vec3 planeY = glm::normalize(upperLeft - lowerLeft);
    const glm::vec3 planeZ = glm::normalize(glm::cross(planeX, planeY));

    // calculate plane rotation using Direction Cosine Matrix (DCM)
    glm::mat3 dcm;
    dcm[0][0] = glm::dot(planeX, glm::vec3(1.f, 0.f, 0.f));
    dcm[0][1] = glm::dot(planeX, glm::vec3(0.f, 1.f, 0.f));
    dcm[0][2] = glm::dot(planeX, glm::vec3(0.f, 0.f, 1.f));

    dcm[1][0] = glm::dot(planeY, glm::vec3(1.f, 0.f, 0.f));
    dcm[1][1] = glm::dot(planeY, glm::vec3(0.f, 1.f, 0.f));
    dcm[1][2] = glm::dot(planeY, glm::vec3(0.f, 0.f, 1.f));

    dcm[2][0] = glm::dot(planeZ, glm::vec3(1.f, 0.f, 0.f));
    dcm[2][1] = glm::dot(planeZ, glm::vec3(0.f, 1.f, 0.f));
    dcm[2][2] = glm::dot(planeZ, glm::vec3(0.f, 0.f, 1.f));

    // invert & transform
    const glm::mat3 invDcm = glm::inverse(dcm);
    const glm::vec3 viewPlaneLowerLeft = invDcm * lowerLeft;
    const glm::vec3 viewPlaneUpperRight = invDcm * upperRight;
    const glm::vec3 eyePos = invDcm * b;

    // nearFactor = near clipping plane / focus plane dist
    const float nearF = std::fabs(nearClip / (viewPlaneLowerLeft.z - eyePos.z));

    _frustum.left = (viewPlaneLowerLeft.x - eyePos.x) * nearF;
    _frustum.right = (viewPlaneUpperRight.x - eyePos.x) * nearF;
    _frustum.bottom = (viewPlaneLowerLeft.y - eyePos.y) * nearF;
    _frustum.top = (viewPlaneUpperRight.y - eyePos.y) * nearF;
    _frustum.nearPlane = nearClip;
    _frustum.farPlane = farClip;

    glm::mat4 complete = glm::mat4(invDcm) * glm::translate(glm::mat4(1.f), -(b + o));
    std::memcpy(&_viewMatrix, glm::value_ptr(complete), sizeof(sgct::mat4));

    // calc frustum matrix
    glm::mat4 frustum = glm::frustum(
        _frustum.left,
        _frustum.right,
        _frustum.bottom,
        _frustum.top,
        _frustum.nearPlane,
        _frustum.farPlane
    );
    std::memcpy(&_projectionMatrix, glm::value_ptr(frustum), sizeof(sgct::mat4));

    _viewProjectionMatrix = _projectionMatrix * _viewMatrix;
}

const mat4& Projection::viewProjectionMatrix() const {
    return _viewProjectionMatrix;
}

const mat4& Projection::viewMatrix() const {
    return _viewMatrix;
}

const mat4& Projection::projectionMatrix() const {
    return _projectionMatrix;
}

} // namespace sgct
