/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/baseviewport.h>

#include <sgct/clustermanager.h>
#include <sgct/profiling.h>
#include <sgct/user.h>
#include <sgct/window.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace sgct {

BaseViewport::BaseViewport(const Window& parent, FrustumMode eye)
    : _parent(parent)
    , _eye(eye)
    , _user(&ClusterManager::instance().defaultUser())
{
    assert(_user);
}

BaseViewport::~BaseViewport() = default;

void BaseViewport::setupViewport(FrustumMode frustum) const {
    ZoneScoped;

    const ivec2 res = _parent.framebufferResolution();
    ivec4 vpCoordinates = ivec4 {
        static_cast<int>(_position.x * res.x),
        static_cast<int>(_position.y * res.y),
        static_cast<int>(_size.x * res.x),
        static_cast<int>(_size.y * res.y)
    };

    if (frustum == FrustumMode::StereoLeft) {
        switch (_parent.stereoMode()) {
            case Window::StereoMode::SideBySide:
                vpCoordinates.x /= 2;
                vpCoordinates.z /= 2;
                break;
            case Window::StereoMode::SideBySideInverted:
                vpCoordinates.x = (vpCoordinates.x / 2) + (vpCoordinates.z / 2);
                vpCoordinates.z = vpCoordinates.z / 2;
                break;
            case Window::StereoMode::TopBottom:
                vpCoordinates.y = (vpCoordinates.y / 2) + (vpCoordinates.w / 2);
                vpCoordinates.w /= 2;
                break;
            case Window::StereoMode::TopBottomInverted:
                vpCoordinates.y /= 2;
                vpCoordinates.w /= 2;
                break;
            default:
                break;
        }
    }
    else {
        switch (_parent.stereoMode()) {
            case Window::StereoMode::SideBySide:
                vpCoordinates.x = (vpCoordinates.x / 2) + (vpCoordinates.z / 2);
                vpCoordinates.z /= 2;
                break;
            case Window::StereoMode::SideBySideInverted:
                vpCoordinates.x /= 2;
                vpCoordinates.z /= 2;
                break;
            case Window::StereoMode::TopBottom:
                vpCoordinates.y /= 2;
                vpCoordinates.w /= 2;
                break;
            case Window::StereoMode::TopBottomInverted:
                vpCoordinates.y = (vpCoordinates.y / 2) + (vpCoordinates.w / 2);
                vpCoordinates.w /= 2;
                break;
            default:
                break;
        }
    }

    glViewport(vpCoordinates.x, vpCoordinates.y, vpCoordinates.z, vpCoordinates.w);
    glScissor(vpCoordinates.x, vpCoordinates.y, vpCoordinates.z, vpCoordinates.w);
}

const Projection& BaseViewport::projection(FrustumMode frustumMode) const {
    switch (frustumMode) {
        case FrustumMode::Mono:        return _monoProj;
        case FrustumMode::StereoLeft:  return _stereoLeftProj;
        case FrustumMode::StereoRight: return _stereoRightProj;
        default:                       throw std::logic_error("Unhandled case label");
    }
}

ProjectionPlane& BaseViewport::projectionPlane() {
    return _projPlane;
}

void BaseViewport::calculateFrustum(FrustumMode mode, float nearClip, float farClip) {
    ZoneScoped;

    switch (mode) {
        case FrustumMode::Mono:
            _monoProj.calculateProjection(
                _user->posMono(),
                _projPlane,
                nearClip,
                farClip
            );
            break;
        case FrustumMode::StereoLeft:
            _stereoLeftProj.calculateProjection(
                _user->posLeftEye(),
                _projPlane,
                nearClip,
                farClip
            );
            break;
        case FrustumMode::StereoRight:
            _stereoRightProj.calculateProjection(
                _user->posRightEye(),
                _projPlane,
                nearClip,
                farClip
            );
            break;
    }
}

void BaseViewport::calculateNonLinearFrustum(FrustumMode mode, float nearClip,
                                             float farClip)
{
    const vec3& pos = _user->posMono();

    switch (mode) {
        case FrustumMode::Mono:
            _monoProj.calculateProjection(pos, _projPlane, nearClip, farClip);
            break;
        case FrustumMode::StereoLeft:
        {
            const vec3 o = vec3 {
                _user->posLeftEye().x - pos.x,
                _user->posLeftEye().y - pos.y,
                _user->posLeftEye().z - pos.z
            };
            _stereoLeftProj.calculateProjection(pos, _projPlane, nearClip, farClip, o);
            break;
        }
        case FrustumMode::StereoRight:
        {
            const vec3 o = vec3 {
                _user->posRightEye().x + pos.x,
                _user->posRightEye().y + pos.y,
                _user->posRightEye().z + pos.z
            };

            _stereoRightProj.calculateProjection(pos, _projPlane, nearClip, farClip, o);
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

void BaseViewport::setEnabled(bool state) {
    _isEnabled = state;
}

bool BaseViewport::isEnabled() const {
    return _isEnabled;
}

void BaseViewport::setPosition(vec2 position) {
    _position = std::move(position);
}

const vec2& BaseViewport::position() const {
    return _position;
}

void BaseViewport::setSize(vec2 size) {
    _size = std::move(size);
}

const vec2& BaseViewport::size() const {
    return _size;
}

void BaseViewport::setUser(User& user) {
    _user = &user;
}

User& BaseViewport::user() const {
    return *_user;
}

FrustumMode BaseViewport::eye() const {
    return _eye;
}

void BaseViewport::setHorizontalFieldOfView(float hFov) {
    if (hFov == horizontalFieldOfViewDegrees()) {
        // The old field of view is the same as the new one, so there is nothing to do
        return;
    }

    const vec3 upperLeft = _projPlane.coordinateUpperLeft();
    const vec3 lowerLeft = _projPlane.coordinateLowerLeft();
    const vec3 upperRight = _projPlane.coordinateUpperRight();

    const double ratio = static_cast<double>(hFov) / horizontalFieldOfViewDegrees();
    const double up = glm::degrees(std::atan(ratio * upperLeft.y / -upperLeft.z));
    const double down = glm::degrees(std::atan(ratio * lowerLeft.y / -lowerLeft.z));
    const double left = glm::degrees(std::atan(ratio * upperLeft.x / -upperLeft.z));
    const double right = glm::degrees(std::atan(ratio * upperRight.x / -upperRight.z));

    setViewPlaneCoordsUsingFOVs(
        static_cast<float>(up),
        static_cast<float>(down),
        static_cast<float>(left),
        static_cast<float>(right),
        _rotation,
        std::abs(upperLeft.z)
    );
}

float BaseViewport::horizontalFieldOfViewDegrees() const {
    // Using the unrotated original viewplane to calculate the field-of-view here
    const float xDist = (_viewPlane.upperRight.x - _viewPlane.upperLeft.x) / 2.f;
    const float zDist = _viewPlane.upperRight.z;
    return (glm::degrees(std::atan(std::abs(xDist / zDist)))) * 2.f;
}

const Window& BaseViewport::window() const {
    return _parent;
}

} // namespace sgct
