/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/SGCTTracker.h>

#include <sgct/Engine.h>
#include <sgct/MessageHandler.h>
#include <sgct/SGCTMutexManager.h>
#include <sgct/SGCTTrackingDevice.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace sgct {

SGCTTracker::SGCTTracker(std::string name) : mName(std::move(name)) {}

void SGCTTracker::setEnabled(bool state) {
    for (std::unique_ptr<SGCTTrackingDevice>& dev : mTrackingDevices) {
        dev->setEnabled(state);
    }
}

void SGCTTracker::addDevice(std::string name, size_t index) {
    mTrackingDevices.push_back(std::make_unique<SGCTTrackingDevice>(index, name));

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "%s: Adding device '%s'\n", mName.c_str(), name.c_str()
    );
}

SGCTTrackingDevice* SGCTTracker::getLastDevice() const {
    return !mTrackingDevices.empty() ? mTrackingDevices.back().get() : nullptr;
}

SGCTTrackingDevice* SGCTTracker::getDevice(size_t index) const {
    return index < mTrackingDevices.size() ? mTrackingDevices[index].get() : nullptr;
}

SGCTTrackingDevice* SGCTTracker::getDevice(const std::string& name) const {
    auto it = std::find_if(
        mTrackingDevices.begin(),
        mTrackingDevices.end(),
        [name](const std::unique_ptr<SGCTTrackingDevice>& dev) {
            return dev->getName() == name;
        }
    );
    if (it != mTrackingDevices.end()) {
        return it->get();
    }
    else {
        return nullptr;
    }
}

SGCTTrackingDevice* SGCTTracker::getDeviceBySensorId(int id) const {
    auto it = std::find_if(
        mTrackingDevices.begin(),
        mTrackingDevices.end(),
        [id](const std::unique_ptr<SGCTTrackingDevice>& dev) {
            return dev->getSensorId() == id;
        }
    );
    if (it != mTrackingDevices.end()) {
        return it->get();
    }
    else {
        return nullptr;
    }
}

void SGCTTracker::setOrientation(glm::quat q) {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);

    // create inverse rotation matrix
    mOrientation = glm::inverse(glm::mat4_cast(q));

    calculateTransform();
}

void SGCTTracker::setOrientation(float xRot, float yRot, float zRot) {
    // create rotation quaternion based on x, y, z rotations
    glm::quat rotQuat;
    rotQuat = glm::rotate(rotQuat, glm::radians(xRot), glm::vec3(1.f, 0.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(yRot), glm::vec3(0.f, 1.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(zRot), glm::vec3(0.f, 0.f, 1.f));
    setOrientation(std::move(rotQuat));
}

void SGCTTracker::setOffset(glm::vec3 offset) {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    mOffset = std::move(offset);
    calculateTransform();
}

void SGCTTracker::setScale(double scaleVal) {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    if (scaleVal > 0.0) {
        mScale = scaleVal;
    }
}

void SGCTTracker::setTransform(glm::mat4 mat) {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    mXform = std::move(mat);
}

glm::mat4 SGCTTracker::getTransform() const { 
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mXform;
}

double SGCTTracker::getScale() const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mScale;
}

int SGCTTracker::getNumberOfDevices() const {
    return static_cast<int>(mTrackingDevices.size());
}

const std::string& SGCTTracker::getName() const {
    return mName;
}

void SGCTTracker::calculateTransform() {
    // create offset translation matrix
    glm::mat4 transMat = glm::translate(glm::mat4(1.f), mOffset);

    // calculate transform
    mXform = transMat * mOrientation;
}

} // namespace sgct
