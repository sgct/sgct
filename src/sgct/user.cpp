/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/user.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace sgct::core {

User::User(std::string name) : mName(std::move(name)) {}

void User::setPos(glm::vec3 pos) {
    mPosMono = std::move(pos);
    updateEyeSeparation();
}

void User::setHeadTracker(std::string trackerName, std::string deviceName) {
    mHeadTrackerName = std::move(trackerName);
    mHeadTrackerDeviceName = std::move(deviceName);
}

void User::setTransform(glm::mat4 transform) {
    mTransform = std::move(transform);
    updateEyeTransform();
}

void User::setOrientation(float xRot, float yRot, float zRot) {
    // create offset translation matrix
    glm::mat4 transMat = glm::translate(glm::mat4(1.f), mPosMono);
    
    mTransform = transMat *
        glm::eulerAngleX(xRot) * glm::eulerAngleY(yRot) * glm::eulerAngleZ(zRot);
    updateEyeTransform();
}

void User::setOrientation(glm::quat q) {
    // create offset translation matrix
    glm::mat4 transMat = glm::translate(glm::mat4(1.f), mPosMono);

    mTransform = transMat * glm::mat4_cast(q);
    updateEyeTransform();
}

void User::setEyeSeparation(float eyeSeparation) {
    mEyeSeparation = eyeSeparation;
    updateEyeSeparation();
}

void User::updateEyeSeparation() {
    glm::vec3 eyeOffsetVec(mEyeSeparation / 2.f, 0.f, 0.f);
    mPosLeftEye = mPosMono - eyeOffsetVec;
    mPosRightEye = mPosMono + eyeOffsetVec;
}

void User::updateEyeTransform() {
    glm::vec4 eyeOffsetVec(mEyeSeparation / 2.f, 0.f, 0.f, 0.f);
    
    glm::vec4 posMono = glm::vec4(0.f, 0.f, 0.f, 1.f);
    glm::vec4 posLeft = posMono - eyeOffsetVec;
    glm::vec4 posRight = posMono + eyeOffsetVec;

    mPosMono = glm::vec3(mTransform * posMono);
    mPosLeftEye = glm::vec3(mTransform * posLeft);
    mPosRightEye = glm::vec3(mTransform * posRight);
}

const glm::vec3& User::getPosMono() const {
    return mPosMono;
}

const glm::vec3& User::getPosLeftEye() const {
    return mPosLeftEye;
}

const glm::vec3& User::getPosRightEye() const {
    return mPosRightEye;
}

float User::getEyeSeparation() const {
    return mEyeSeparation;
}

const std::string& User::getHeadTrackerName() const {
    return mHeadTrackerName;
}

const std::string& User::getHeadTrackerDeviceName() const {
    return mHeadTrackerDeviceName;
}

const std::string& User::getName() const {
    return mName;
}

bool User::isTracked() const {    
    return !(mHeadTrackerDeviceName.empty() || mHeadTrackerName.empty());
}

} // namespace sgct::core
