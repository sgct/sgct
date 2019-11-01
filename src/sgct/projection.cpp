/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/projection.h>

#include <sgct/projectionplane.h>
#include <glm/gtc/matrix_transform.hpp>

namespace sgct::core {

void Projection::calculateProjection(glm::vec3 base, const ProjectionPlane& proj,
                                     float nearClip, float farClip, glm::vec3 offset)
{
    const glm::vec3 lowerLeft = proj.getCoordinateLowerLeft();
    const glm::vec3 upperLeft = proj.getCoordinateUpperLeft();
    const glm::vec3 upperRight = proj.getCoordinateUpperRight();

    // calculate viewplane's internal coordinate system bases
    const glm::vec3 planeX = glm::normalize(upperRight - upperLeft);
    const glm::vec3 planeY = glm::normalize(upperLeft - lowerLeft);
    const glm::vec3 planeZ = glm::normalize(glm::cross(planeX, planeY));

    // calculate plane rotation using Direction Cosine Matrix (DCM)
    glm::mat3 DCM(1.f); // init as identity matrix
    DCM[0][0] = glm::dot(planeX, glm::vec3(1.f, 0.f, 0.f));
    DCM[0][1] = glm::dot(planeX, glm::vec3(0.f, 1.f, 0.f));
    DCM[0][2] = glm::dot(planeX, glm::vec3(0.f, 0.f, 1.f));

    DCM[1][0] = glm::dot(planeY, glm::vec3(1.f, 0.f, 0.f));
    DCM[1][1] = glm::dot(planeY, glm::vec3(0.f, 1.f, 0.f));
    DCM[1][2] = glm::dot(planeY, glm::vec3(0.f, 0.f, 1.f));

    DCM[2][0] = glm::dot(planeZ, glm::vec3(1.f, 0.f, 0.f));
    DCM[2][1] = glm::dot(planeZ, glm::vec3(0.f, 1.f, 0.f));
    DCM[2][2] = glm::dot(planeZ, glm::vec3(0.f, 0.f, 1.f));

    // invert & transform
    const glm::mat3 invDCM = glm::inverse(DCM);
    const glm::vec3 viewPlaneLowerLeft = invDCM * lowerLeft;
    const glm::vec3 viewPlaneUpperRight = invDCM * upperRight;
    const glm::vec3 eyePos = invDCM * base;

    // nearFactor = near clipping plane / focus plane dist
    const float nearF = abs(nearClip / (viewPlaneLowerLeft.z - eyePos.z));

    _frustum.left = (viewPlaneLowerLeft.x - eyePos.x) * nearF;
    _frustum.right = (viewPlaneUpperRight.x - eyePos.x) * nearF;
    _frustum.bottom = (viewPlaneLowerLeft.y - eyePos.y) * nearF;
    _frustum.top = (viewPlaneUpperRight.y - eyePos.y) * nearF;
    _frustum.nearPlane = nearClip;
    _frustum.farPlane = farClip;

    _viewMatrix = glm::mat4(invDCM) * glm::translate(glm::mat4(1.f), -(base + offset));

    // calc frustum matrix
    _projectionMatrix = glm::frustum(
        _frustum.left,
        _frustum.right,
        _frustum.bottom,
        _frustum.top,
        _frustum.nearPlane,
        _frustum.farPlane
    );

    _viewProjectionMatrix = _projectionMatrix * _viewMatrix;
}

const glm::mat4& Projection::getViewProjectionMatrix() const {
    return _viewProjectionMatrix;
}

const glm::mat4& Projection::getViewMatrix() const {
    return _viewMatrix;
}

const glm::mat4& Projection::getProjectionMatrix() const {
    return _projectionMatrix;
}

} // namespace sgct::core
