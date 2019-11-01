/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/baseviewport.h>

#include <sgct/clustermanager.h>
#include <sgct/user.h>

namespace sgct::core {

BaseViewport::BaseViewport()
    : _user(ClusterManager::instance().getDefaultUser())
{}

void BaseViewport::setPos(glm::vec2 position) {
    _position = std::move(position);
}

void BaseViewport::setSize(glm::vec2 size) {
    _size = std::move(size);
}

void BaseViewport::setEnabled(bool state) {
    _isEnabled = state;
}

bool BaseViewport::isEnabled() const {
    return _isEnabled;
}

void BaseViewport::setEye(Frustum::Mode eye) {
    _eye = eye;
}

const glm::vec2& BaseViewport::getPosition() const {
    return _position;
}

const glm::vec2& BaseViewport::getSize() const {
    return _size;
}

void BaseViewport::setUser(User& user) {
    _user = user;
}

User& BaseViewport::getUser() const {
    return _user;
}

Frustum::Mode BaseViewport::getEye() const {
    return _eye;
}

const Projection& BaseViewport::getProjection(Frustum::Mode frustumMode) const {
    switch (frustumMode) {
        default:
        case Frustum::Mode::MonoEye:
            return _projections.mono;
        case Frustum::Mode::StereoLeftEye:
            return _projections.stereoLeft;
        case Frustum::Mode::StereoRightEye:
            return _projections.stereoRight;
    }
}

ProjectionPlane& BaseViewport::getProjectionPlane() {
    return _projectionPlane;
}

void BaseViewport::setUserName(std::string userName) {
    _userName = std::move(userName);
    linkUserName();
}

void BaseViewport::linkUserName() {
    User* user = ClusterManager::instance().getUser(_userName);
    if (user) {
        _user = *user;
    }
}

void BaseViewport::calculateFrustum(Frustum::Mode mode, float nearClip, float farClip) {
    switch (mode) {
        default:
        case Frustum::Mode::MonoEye:
            _projections.mono.calculateProjection(
                _user.getPosMono(),
                _projectionPlane,
                nearClip,
                farClip
            );
            break;
        case Frustum::Mode::StereoLeftEye:
            _projections.stereoLeft.calculateProjection(
                _user.getPosLeftEye(),
                _projectionPlane,
                nearClip,
                farClip
            );
            break;
        case Frustum::Mode::StereoRightEye:
            _projections.stereoRight.calculateProjection(
                _user.getPosRightEye(),
                _projectionPlane,
                nearClip,
                farClip
            );
            break;
    }
}

void BaseViewport::calculateNonLinearFrustum(Frustum::Mode mode, float nearClip,
                                             float farClip)
{
    const glm::vec3& eyePos = _user.getPosMono();

    switch (mode) {
        case Frustum::Mode::MonoEye:
            _projections.mono.calculateProjection(
                eyePos,
                _projectionPlane,
                nearClip,
                farClip,
                _user.getPosMono() - eyePos
            );
            break;
        case Frustum::Mode::StereoLeftEye:
            _projections.stereoLeft.calculateProjection(
                eyePos,
                _projectionPlane,
                nearClip,
                farClip,
                _user.getPosLeftEye() - eyePos
            );
            break;
        case Frustum::Mode::StereoRightEye:
            _projections.stereoRight.calculateProjection(
                eyePos,
                _projectionPlane,
                nearClip,
                farClip,
                _user.getPosRightEye() - eyePos
            );
            break;
    }
}

void BaseViewport::setViewPlaneCoordsUsingFOVs(float up, float down, float left,
                                               float right, glm::quat rot, float dist)
{
    _rotation = std::move(rot);

    _viewPlane.lowerLeft.x = dist * tan(glm::radians(left));
    _viewPlane.lowerLeft.y = dist * tan(glm::radians(down));
    _viewPlane.lowerLeft.z = -dist;

    _viewPlane.upperLeft.x = dist * tan(glm::radians(left));
    _viewPlane.upperLeft.y = dist * tan(glm::radians(up));
    _viewPlane.upperLeft.z = -dist;

    _viewPlane.upperRight.x = dist * tan(glm::radians(right));
    _viewPlane.upperRight.y = dist * tan(glm::radians(up));
    _viewPlane.upperRight.z = -dist;

    _projectionPlane.setCoordinateLowerLeft(_rotation * _viewPlane.lowerLeft);
    _projectionPlane.setCoordinateUpperLeft(_rotation * _viewPlane.upperLeft);
    _projectionPlane.setCoordinateUpperRight(_rotation * _viewPlane.upperRight);
}

void BaseViewport::updateFovToMatchAspectRatio(float oldRatio, float newRatio) {
    _viewPlane.lowerLeft.x *= newRatio / oldRatio;
    _viewPlane.upperLeft.x *= newRatio / oldRatio;
    _viewPlane.upperRight.x *= newRatio / oldRatio;
    _projectionPlane.setCoordinateLowerLeft(_rotation * _viewPlane.lowerLeft);
    _projectionPlane.setCoordinateUpperLeft(_rotation * _viewPlane.upperLeft);
    _projectionPlane.setCoordinateUpperRight(_rotation * _viewPlane.upperRight);
}

float BaseViewport::getHorizontalFieldOfViewDegrees() const {
    const float xDist = (_projectionPlane.getCoordinateUpperRight().x -
        _projectionPlane.getCoordinateUpperLeft().x) / 2;
    const float zDist = _projectionPlane.getCoordinateUpperRight().z;
    return (glm::degrees(atan(abs(xDist / zDist)))) * 2;
}

void BaseViewport::setHorizontalFieldOfView(float hFov, float aspectRatio) {
    const float hFov2 = hFov / 2.f;
    const float zDist = abs(_projectionPlane.getCoordinateUpperRight().z);

    const float projDimX = zDist * tan(glm::radians(hFov2));
    const float projDimY = projDimX / aspectRatio;
    const float vAngle = glm::degrees(atan(projDimY / zDist));

    setViewPlaneCoordsUsingFOVs(vAngle, -vAngle, -hFov2, hFov2, _rotation, zDist);
}

} // namespace sgct::core
