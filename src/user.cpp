/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/user.h>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace sgct {

User::User(const config::User& user)
    : _name(user.name.value_or(std::string("default")))
    , _eyeSeparation(user.eyeSeparation.value_or(0.06f))
    , _posMono(user.position.value_or(glm::vec3(0.f, 0.f, 0.f)))
    , _transform(user.transformation.value_or(glm::mat4(1.f)))
    , _headTrackerDeviceName(user.tracking ? user.tracking->device : std::string())
    , _headTrackerName(user.tracking ? user.tracking->tracker : std::string())
{
    updateEyeTransform();
    updateEyeSeparation();
}

void User::setPos(glm::vec3 pos) {
    _posMono = std::move(pos);
    updateEyeSeparation();
}

void User::setHeadTracker(std::string trackerName, std::string deviceName) {
    _headTrackerName = std::move(trackerName);
    _headTrackerDeviceName = std::move(deviceName);
}

void User::setTransform(glm::mat4 transform) {
    _transform = transform;
    updateEyeTransform();
}

void User::setOrientation(float x, float y, float z) {
    const glm::mat4 trans = glm::translate(glm::mat4(1.f), _posMono);
    _transform = trans * glm::eulerAngleX(x) * glm::eulerAngleY(y) * glm::eulerAngleZ(z);
    updateEyeTransform();
}

void User::setOrientation(glm::quat q) {
    const glm::mat4 trans = glm::translate(glm::mat4(1.f), _posMono);
    _transform = trans * glm::mat4_cast(q);
    updateEyeTransform();
}

void User::setEyeSeparation(float eyeSeparation) {
    _eyeSeparation = eyeSeparation;
    updateEyeSeparation();
}

void User::updateEyeSeparation() {
    const glm::vec3 eyeOffsetVec(_eyeSeparation / 2.f, 0.f, 0.f);
    _posLeftEye = _posMono - eyeOffsetVec;
    _posRightEye = _posMono + eyeOffsetVec;
}

void User::updateEyeTransform() {
    const glm::vec4 eyeOffsetVec(_eyeSeparation / 2.f, 0.f, 0.f, 0.f);
    const glm::vec4 posMono = glm::vec4(_posMono, 1.f);
    const glm::vec4 posLeft = posMono - eyeOffsetVec;
    const glm::vec4 posRight = posMono + eyeOffsetVec;

    _posMono = glm::vec3(_transform * posMono);
    _posLeftEye = glm::vec3(_transform * posLeft);
    _posRightEye = glm::vec3(_transform * posRight);
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
