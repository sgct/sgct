/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/baseviewport.h>

#include <sgct/clustermanager.h>
#include <sgct/format.h>
#include <sgct/log.h>
#include <sgct/profiling.h>
#include <sgct/user.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <stdexcept>
#include <type_traits>

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

void BaseViewport::setEye(FrustumMode eye) {
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

FrustumMode BaseViewport::eye() const {
    return _eye;
}

const Projection& BaseViewport::projection(FrustumMode frustumMode) const {
    switch (frustumMode) {
        case FrustumMode::Mono:        return _monoProj;
        case FrustumMode::StereoLeft:  return _stereoLeftProj;
        case FrustumMode::StereoRight: return _stereoRightProj;
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
            Log::Warning(std::format("Could not find user with name '{}'", _userName));
        }
    }
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

void BaseViewport::setupViewport(FrustumMode frustum) const {
    ZoneScoped;

    const ivec2 res = _parent->framebufferResolution();
    ivec4 vpCoordinates = ivec4{
        static_cast<int>(position().x * res.x),
        static_cast<int>(position().y * res.y),
        static_cast<int>(size().x * res.x),
        static_cast<int>(size().y * res.y)
    };

    const Window::StereoMode sm = _parent->stereoMode();
    if (frustum == FrustumMode::StereoLeft) {
        switch (sm) {
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
        switch (sm) {
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

void BaseViewport::calculateNonLinearFrustum(FrustumMode mode, float nearClip,
                                             float farClip)
{
    const vec3& eyePos = _user->posMono();

    switch (mode) {
        case FrustumMode::Mono:
            _monoProj.calculateProjection(
                eyePos,
                _projPlane,
                nearClip,
                farClip
            );
            break;
        case FrustumMode::StereoLeft:
        {
            const vec3 p {
                _user->posLeftEye().x - eyePos.x,
                _user->posLeftEye().y - eyePos.y,
                _user->posLeftEye().z - eyePos.z
            };
            _stereoLeftProj.calculateProjection(eyePos, _projPlane, nearClip, farClip, p);
            break;
        }
        case FrustumMode::StereoRight:
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
    // Using the unrotated original viewplane to calculate the field-of-view here
    const float xDist = (_viewPlane.upperRight.x - _viewPlane.upperLeft.x) / 2.f;
    const float zDist = _viewPlane.upperRight.z;
    return (glm::degrees(std::atan(std::abs(xDist / zDist)))) * 2.f;
}

const Window* BaseViewport::parent() const {
    return _parent;
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

} // namespace sgct
