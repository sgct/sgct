/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/SGCTUser.h>

#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace sgct_core {

SGCTUser::SGCTUser(std::string name) : mName(std::move(name)) {}

void SGCTUser::setPos(glm::vec3 pos) {
    mPosMono = std::move(pos);
    updateEyeSeparation();
}

void SGCTUser::setHeadTracker(std::string trackerName, std::string deviceName) {
    mHeadTrackerName = std::move(trackerName);
    mHeadTrackerDeviceName = std::move(deviceName);
}

void SGCTUser::setTransform(glm::mat4 transform) {
    mTransform = std::move(transform);
    updateEyeTransform();
}

void SGCTUser::setOrientation(float xRot, float yRot, float zRot) {
    // create offset translation matrix
    glm::mat4 transMat = glm::translate(glm::mat4(1.f), mPosMono);
    
    mTransform = transMat *
        glm::eulerAngleX(xRot) * glm::eulerAngleY(yRot) * glm::eulerAngleZ(zRot);
    updateEyeTransform();
}

void SGCTUser::setOrientation(glm::quat q) {
    // create offset translation matrix
    glm::mat4 transMat = glm::translate(glm::mat4(1.f), mPosMono);

    mTransform = transMat * glm::mat4_cast(q);
    updateEyeTransform();
}

void SGCTUser::setEyeSeparation(float eyeSeparation) {
    mEyeSeparation = eyeSeparation;
    updateEyeSeparation();
}

void SGCTUser::updateEyeSeparation() {
    glm::vec3 eyeOffsetVec(mEyeSeparation / 2.f, 0.f, 0.f);
    mPosLeftEye = mPosMono - eyeOffsetVec;
    mPosRightEye = mPosMono + eyeOffsetVec;
}

void SGCTUser::updateEyeTransform() {
    glm::vec4 eyeOffsetVec(mEyeSeparation / 2.f, 0.f, 0.f, 0.f);
    
    glm::vec4 posMono = glm::vec4(0.f, 0.f, 0.f, 1.f);
    glm::vec4 posLeft = posMono - eyeOffsetVec;
    glm::vec4 posRight = posMono + eyeOffsetVec;

    mPosMono = glm::vec3(mTransform * posMono);
    mPosLeftEye = glm::vec3(mTransform * posLeft);
    mPosRightEye = glm::vec3(mTransform * posRight);
}

const glm::vec3& SGCTUser::getPosMono() const {
    return mPosMono;
}

const glm::vec3& SGCTUser::getPosLeftEye() const {
    return mPosLeftEye;
}

const glm::vec3& SGCTUser::getPosRightEye() const {
    return mPosRightEye;
}

float SGCTUser::getEyeSeparation() const {
    return mEyeSeparation;
}

const std::string& SGCTUser::getHeadTrackerName() const {
    return mHeadTrackerName;
}

const std::string& SGCTUser::getHeadTrackerDeviceName() const {
    return mHeadTrackerDeviceName;
}

const std::string& SGCTUser::getName() const {
    return mName;
}

bool SGCTUser::isTracked() const {    
    return !(mHeadTrackerDeviceName.empty() || mHeadTrackerName.empty());
}

} // namespace sgct_core
