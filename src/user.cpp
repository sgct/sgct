/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/user.h>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace sgct {

User::User(const config::User& user)
    : _name(user.name.value_or(std::string("default")))
    , _eyeSeparation(user.eyeSeparation.value_or(0.06f))
    , _posMono(user.position.value_or(vec3{ 0.f, 0.f, 0.f }))
    , _transform(user.transformation.value_or(mat4(1.0)))
    , _headTrackerDeviceName(user.tracking ? user.tracking->device : std::string())
    , _headTrackerName(user.tracking ? user.tracking->tracker : std::string())
{
    updateEyeTransform();
    updateEyeSeparation();
}

void User::setPos(vec3 pos) {
    _posMono = std::move(pos);
    updateEyeSeparation();
}

void User::setHeadTracker(std::string trackerName, std::string deviceName) {
    _headTrackerName = std::move(trackerName);
    _headTrackerDeviceName = std::move(deviceName);
}

void User::setTransform(mat4 transform) {
    _transform = std::move(transform);
    updateEyeTransform();
}

void User::setOrientation(float x, float y, float z) {
    const glm::mat4 trans = glm::translate(glm::mat4(1.f), glm::make_vec3(&_posMono.x));
    const glm::mat4 c =
        trans * glm::eulerAngleX(x) * glm::eulerAngleY(y) * glm::eulerAngleZ(z);
    std::memcpy(&_transform, glm::value_ptr(c), sizeof(sgct::mat4));
    updateEyeTransform();
}

void User::setOrientation(quat q) {
    const glm::mat4 trans = glm::translate(glm::mat4(1.f), glm::make_vec3(&_posMono.x));
    const glm::mat4 c = trans * glm::mat4_cast(glm::make_quat(&q.x));
    std::memcpy(&_transform, glm::value_ptr(c), sizeof(sgct::mat4));
    updateEyeTransform();
}

void User::setEyeSeparation(float eyeSeparation) {
    _eyeSeparation = eyeSeparation;
    updateEyeSeparation();
}

void User::updateEyeSeparation() {
    const glm::vec3 eyeOffsetVec(_eyeSeparation / 2.f, 0.f, 0.f);
    _posLeftEye.x = _posMono.x - eyeOffsetVec.x;
    _posLeftEye.y = _posMono.y - eyeOffsetVec.y;
    _posLeftEye.z = _posMono.z - eyeOffsetVec.z;
    _posRightEye.x = _posMono.x + eyeOffsetVec.x;
    _posRightEye.y = _posMono.y + eyeOffsetVec.y;
    _posRightEye.z = _posMono.z + eyeOffsetVec.z;
}

void User::updateEyeTransform() {
    const glm::vec4 eyeOffsetVec(_eyeSeparation / 2.f, 0.f, 0.f, 0.f);
    const glm::vec4 posMono = glm::vec4(_posMono.x, _posMono.y, _posMono.z, 1.f);
    const glm::vec4 posLeft = posMono - eyeOffsetVec;
    const glm::vec4 posRight = posMono + eyeOffsetVec;

    const glm::mat4 trans = glm::make_mat4(_transform.values.data());

    const glm::vec4 mono = trans * posMono;
    const glm::vec4 left = trans * posLeft;
    const glm::vec4 right = trans * posRight;
    _posMono = vec3(mono.x, mono.y, mono.z);
    _posLeftEye = vec3(left.x, left.y, left.z);
    _posRightEye = vec3(right.x, right.y, right.z);
}

const vec3& User::posMono() const {
    return _posMono;
}

const vec3& User::posLeftEye() const {
    return _posLeftEye;
}

const vec3& User::posRightEye() const {
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
