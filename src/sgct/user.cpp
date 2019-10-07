/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/user.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace sgct::core {

User::User(std::string name) : _name(std::move(name)) {}

void User::setPos(glm::vec3 pos) {
    _posMono = std::move(pos);
    updateEyeSeparation();
}

void User::setHeadTracker(std::string trackerName, std::string deviceName) {
    _headTrackerName = std::move(trackerName);
    _headTrackerDeviceName = std::move(deviceName);
}

void User::setTransform(glm::mat4 transform) {
    _transform = std::move(transform);
    updateEyeTransform();
}

void User::setOrientation(float xRot, float yRot, float zRot) {
    // create offset translation matrix
    glm::mat4 transMat = glm::translate(glm::mat4(1.f), _posMono);
    
    _transform = transMat *
        glm::eulerAngleX(xRot) * glm::eulerAngleY(yRot) * glm::eulerAngleZ(zRot);
    updateEyeTransform();
}

void User::setOrientation(glm::quat q) {
    // create offset translation matrix
    glm::mat4 transMat = glm::translate(glm::mat4(1.f), _posMono);

    _transform = transMat * glm::mat4_cast(q);
    updateEyeTransform();
}

void User::setEyeSeparation(float eyeSeparation) {
    _eyeSeparation = eyeSeparation;
    updateEyeSeparation();
}

void User::updateEyeSeparation() {
    glm::vec3 eyeOffsetVec(_eyeSeparation / 2.f, 0.f, 0.f);
    _posLeftEye = _posMono - eyeOffsetVec;
    _posRightEye = _posMono + eyeOffsetVec;
}

void User::updateEyeTransform() {
    glm::vec4 eyeOffsetVec(_eyeSeparation / 2.f, 0.f, 0.f, 0.f);
    
    glm::vec4 posMono = glm::vec4(0.f, 0.f, 0.f, 1.f);
    glm::vec4 posLeft = posMono - eyeOffsetVec;
    glm::vec4 posRight = posMono + eyeOffsetVec;

    _posMono = glm::vec3(_transform * posMono);
    _posLeftEye = glm::vec3(_transform * posLeft);
    _posRightEye = glm::vec3(_transform * posRight);
}

const glm::vec3& User::getPosMono() const {
    return _posMono;
}

const glm::vec3& User::getPosLeftEye() const {
    return _posLeftEye;
}

const glm::vec3& User::getPosRightEye() const {
    return _posRightEye;
}

float User::getEyeSeparation() const {
    return _eyeSeparation;
}

const std::string& User::getHeadTrackerName() const {
    return _headTrackerName;
}

const std::string& User::getHeadTrackerDeviceName() const {
    return _headTrackerDeviceName;
}

const std::string& User::getName() const {
    return _name;
}

bool User::isTracked() const {    
    return !(_headTrackerDeviceName.empty() || _headTrackerName.empty());
}

} // namespace sgct::core
