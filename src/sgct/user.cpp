/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/user.h>

#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace sgct {

namespace {
    template <typename From, typename To>
    To fromGLM(From v) {
        To r;
        std::memcpy(&r, glm::value_ptr(v), sizeof(To));
        return r;
    }
} // namespace

User::User(std::string name) : _name(std::move(name)) {}

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
    glm::mat4 trans = glm::translate(glm::mat4(1.f), glm::make_vec3(&_posMono.x));
    _transform = fromGLM<glm::mat4, mat4>(
        trans * glm::eulerAngleX(x) * glm::eulerAngleY(y) * glm::eulerAngleZ(z)
    );
    updateEyeTransform();
}

void User::setOrientation(quat q) {
    glm::mat4 trans = glm::translate(glm::mat4(1.f), glm::make_vec3(&_posMono.x));
    _transform = fromGLM<glm::mat4, mat4>(trans * glm::mat4_cast(glm::make_quat(&q.x)));
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
    const glm::vec4 posMono = glm::vec4(0.f, 0.f, 0.f, 1.f);
    const glm::vec4 posLeft = posMono - eyeOffsetVec;
    const glm::vec4 posRight = posMono + eyeOffsetVec;

    const glm::mat4 trans = glm::make_mat4(_transform.values);

    _posMono = fromGLM<glm::vec3, vec3>(glm::vec3(trans * posMono));
    _posLeftEye = fromGLM<glm::vec3, vec3>(glm::vec3(trans * posLeft));
    _posRightEye = fromGLM<glm::vec3, vec3>(glm::vec3(trans * posRight));
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
