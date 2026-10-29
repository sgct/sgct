/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/user.h>

#include <sgct/glm.h>
#include <glm/gtx/euler_angles.hpp>

namespace sgct {

User::User(const config::User& user)
    : _name(user.name.value_or(std::string("default")))
    , _eyeSeparation(user.eyeSeparation.value_or(0.06f))
    , _position(user.position.value_or(glm::vec3(0.f, 0.f, 0.f)))
    , _transform(user.transformation.value_or(glm::mat4(1.f)))
    , _headTrackerDeviceName(user.tracking ? user.tracking->device : std::string())
    , _headTrackerName(user.tracking ? user.tracking->tracker : std::string())
{
    updateEyePositions();
}

void User::setPos(glm::vec3 pos) {
    _position = std::move(pos);
    updateEyePositions();
}

void User::setHeadTracker(std::string trackerName, std::string deviceName) {
    _headTrackerName = std::move(trackerName);
    _headTrackerDeviceName = std::move(deviceName);
}

void User::setTransform(glm::mat4 transform) {
    _transform = transform;
    updateEyePositions();
}

void User::setOrientation(float x, float y, float z) {
    _transform = glm::eulerAngleX(x) * glm::eulerAngleY(y) * glm::eulerAngleZ(z);
    updateEyePositions();
}

void User::setOrientation(glm::quat q) {
    _transform = glm::mat4_cast(q);
    updateEyePositions();
}

void User::setEyeSeparation(float eyeSeparation) {
    _eyeSeparation = eyeSeparation;
    updateEyePositions();
}

void User::updateEyePositions() {
    // The eyes are placed around the origin and transformed by the head transform before
    // the user's position is applied as a linear offset. Always computing the positions
    // from scratch prevents repeated calls from accumulating the transform
    const glm::vec4 eyeOffsetVec(_eyeSeparation / 2.f, 0.f, 0.f, 0.f);
    const glm::vec4 origin = glm::vec4(0.f, 0.f, 0.f, 1.f);
    _posMono = _position + glm::vec3(_transform * origin);
    _posLeftEye = _position + glm::vec3(_transform * (origin - eyeOffsetVec));
    _posRightEye = _position + glm::vec3(_transform * (origin + eyeOffsetVec));
}

const glm::vec3& User::posMono() const {
    return _posMono;
}

const glm::vec3& User::posLeftEye() const {
    return _posLeftEye;
}

const glm::vec3& User::posRightEye() const {
    return _posRightEye;
}

float User::eyeSeparation() const {
    return _eyeSeparation;
}

const std::string& User::headTrackerName() const {
    return _headTrackerName;
}

const std::string& User::headTrackerDeviceName() const {
    return _headTrackerDeviceName;
}

const std::string& User::name() const {
    return _name;
}

bool User::isTracked() const {
    return !(_headTrackerDeviceName.empty() || _headTrackerName.empty());
}

} // namespace sgct
