/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/baseviewport.h>

#include <sgct/clustermanager.h>
#include <sgct/fmt.h>
#include <sgct/log.h>
#include <sgct/profiling.h>
#include <sgct/user.h>
#include <stdexcept>
#include <type_traits>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

namespace sgct {

BaseViewport::BaseViewport(const Window* parent)
    : _parent(parent)
    , _user(&ClusterManager::instance().defaultUser())
{}

BaseViewport::~BaseViewport() = default;

void BaseViewport::setPos(vec2 position) {
    _position = std::move(position);
}

void BaseViewport::setSize(vec2 size) {
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

const vec2& BaseViewport::position() const {
    return _position;
}

const vec2& BaseViewport::size() const {
    return _size;
}

void BaseViewport::setUser(User* user) {
    _user = user;
}

User& BaseViewport::user() const {
    return *_user;
}

const Window& BaseViewport::window() const {
    return *_parent;
}

Frustum::Mode BaseViewport::eye() const {
    return _eye;
}

const Projection& BaseViewport::projection(Frustum::Mode frustumMode) const {
    switch (frustumMode) {
        case Frustum::Mode::MonoEye:        return _monoProj;
        case Frustum::Mode::StereoLeftEye:  return _stereoLeftProj;
        case Frustum::Mode::StereoRightEye: return _stereoRightProj;
        default:                           throw std::logic_error("Unhandled case label");
    }
}

ProjectionPlane& BaseViewport::projectionPlane() {
    return _projPlane;
}

void BaseViewport::setUserName(std::string userName) {
    _userName = std::move(userName);
    linkUserName();
}

void BaseViewport::linkUserName() {
    ZoneScoped;

    if (!_userName.empty()) {
        User* user = ClusterManager::instance().user(_userName);
        if (user) {
            // If the user name is not empty, the User better exists
            _user = user;
        }
        else {
            Log::Warning(fmt::format("Could not find user with name '{}'", _userName));
        }
    }
}

void BaseViewport::calculateFrustum(Frustum::Mode mode, float nearClip, float farClip) {
    ZoneScoped;

    switch (mode) {
        case Frustum::Mode::MonoEye:
            _monoProj.calculateProjection(
                _user->posMono(),
                _projPlane,
                nearClip,
                farClip
            );
            break;
        case Frustum::Mode::StereoLeftEye:
            _stereoLeftProj.calculateProjection(
                _user->posLeftEye(),
                _projPlane,
                nearClip,
                farClip
            );
            break;
        case Frustum::Mode::StereoRightEye:
            _stereoRightProj.calculateProjection(
                _user->posRightEye(),
                _projPlane,
                nearClip,
                farClip
            );
            break;
    }
}

void BaseViewport::calculateNonLinearFrustum(Frustum::Mode mode, float nearClip,
                                             float farClip)
{
    const vec3& eyePos = _user->posMono();

    switch (mode) {
        case Frustum::Mode::MonoEye:
            _monoProj.calculateProjection(
                eyePos,
                _projPlane,
                nearClip,
                farClip
            );
            break;
        case Frustum::Mode::StereoLeftEye:
        {
            const vec3 p {
                _user->posLeftEye().x - eyePos.x,
                _user->posLeftEye().y - eyePos.y,
                _user->posLeftEye().z - eyePos.z
            };
            _stereoLeftProj.calculateProjection(eyePos, _projPlane, nearClip, farClip, p);
            break;
        }
        case Frustum::Mode::StereoRightEye:
        {
            const vec3 p {
                _user->posRightEye().x + eyePos.x,
                _user->posRightEye().y + eyePos.y,
                _user->posRightEye().z + eyePos.z
            };

            _stereoRightProj.calculateProjection(
                eyePos,
                _projPlane,
                nearClip,
                farClip,
                p
            );
            break;
        }
    }
}

void BaseViewport::setViewPlaneCoordsUsingFOVs(float up, float down, float left,
                                               float right, quat rot, float dist)
{
    _rotation = std::move(rot);

    _viewPlane.lowerLeft.x = dist * std::tan(glm::radians(left));
    _viewPlane.lowerLeft.y = dist * std::tan(glm::radians(down));
    _viewPlane.lowerLeft.z = -dist;

    _viewPlane.upperLeft.x = dist * std::tan(glm::radians(left));
    _viewPlane.upperLeft.y = dist * std::tan(glm::radians(up));
    _viewPlane.upperLeft.z = -dist;

    _viewPlane.upperRight.x = dist * std::tan(glm::radians(right));
    _viewPlane.upperRight.y = dist * std::tan(glm::radians(up));
    _viewPlane.upperRight.z = -dist;

    _projPlane.setCoordinates(
        _rotation * _viewPlane.lowerLeft,
        _rotation * _viewPlane.upperLeft,
        _rotation * _viewPlane.upperRight
    );
}

void BaseViewport::updateFovToMatchAspectRatio(float oldRatio, float newRatio) {
    _viewPlane.lowerLeft.x *= newRatio / oldRatio;
    _viewPlane.upperLeft.x *= newRatio / oldRatio;
    _viewPlane.upperRight.x *= newRatio / oldRatio;

    _projPlane.setCoordinates(
        _rotation * _viewPlane.lowerLeft,
        _rotation * _viewPlane.upperLeft,
        _rotation * _viewPlane.upperRight
    );
}

float BaseViewport::horizontalFieldOfViewDegrees() const {
    const float xDist = (_projPlane.coordinateUpperRight().x -
        _projPlane.coordinateUpperLeft().x) / 2.f;
    const float zDist = _projPlane.coordinateUpperRight().z;
    return (glm::degrees(std::atan(std::fabs(xDist / zDist)))) * 2.f;
}

void BaseViewport::setHorizontalFieldOfView(float hFov) {
    const vec3 upperLeft = _projPlane.coordinateUpperLeft();
    const vec3 lowerLeft = _projPlane.coordinateLowerLeft();
    const vec3 upperRight = _projPlane.coordinateUpperRight();

    const float ratio = hFov / horizontalFieldOfViewDegrees();
    const float up = glm::degrees(std::atan(ratio * upperLeft.y / -upperLeft.z));
    const float down = glm::degrees(std::atan(ratio * lowerLeft.y / -lowerLeft.z));
    const float left = glm::degrees(std::atan(ratio * upperLeft.x / -upperLeft.z));
    const float right = glm::degrees(std::atan(ratio * upperRight.x / -upperRight.z));

    setViewPlaneCoordsUsingFOVs(up, down, left, right, _rotation, std::fabs(upperLeft.z));
}

} // namespace sgct
