/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/SGCTTracker.h>
#include <sgct/MessageHandler.h>
#include <sgct/Engine.h>

extern GLFWmutex gTrackingMutex;

namespace sgct {

SGCTTracker::SGCTTracker(std::string name) : mName(std::move(name)) {}

SGCTTracker::~SGCTTracker() {
    for (size_t i = 0; i < mTrackingDevices.size(); i++) {
        if (mTrackingDevices[i] != nullptr) {
            delete mTrackingDevices[i];
            mTrackingDevices[i] = nullptr;
        }
    }
}

void SGCTTracker::setEnabled(bool state) {
    for (size_t i = 0; i < mTrackingDevices.size(); i++) {
        mTrackingDevices[i]->setEnabled(state);
    }
}

void SGCTTracker::addDevice(std::string name, size_t index) {
    SGCTTrackingDevice* td = new SGCTTrackingDevice(index, name);

    mTrackingDevices.push_back(td);

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "%s: Adding device '%s'...\n",
        mName.c_str(),
        name.c_str()
    );
}

SGCTTrackingDevice* SGCTTracker::getLastDevicePtr() {
    return mTrackingDevices.size() > 0 ? mTrackingDevices.back() : nullptr;
}

SGCTTrackingDevice* SGCTTracker::getDevicePtr(size_t index) {
    return index < mTrackingDevices.size() ? mTrackingDevices[index] : nullptr;
}

SGCTTrackingDevice* SGCTTracker::getDevicePtr(const char* name) {
    for (size_t i = 0; i < mTrackingDevices.size(); i++) {
        if (name == mTrackingDevices[i]->getName()) {
            return mTrackingDevices[i];
        }
    }

    //if not found
    return nullptr;
}

SGCTTrackingDevice* SGCTTracker::getDevicePtrBySensorId(int id) {
    for (size_t i = 0; i < mTrackingDevices.size(); i++) {
        if (mTrackingDevices[i]->getSensorId() == id) {
            return mTrackingDevices[i];
        }
    }

    return nullptr;
}

/*!
Set the orientation as quaternion
*/
void SGCTTracker::setOrientation(glm::quat q) {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);

    //create inverse rotation matrix
    mOrientation = glm::inverse(glm::mat4_cast(q));

    calculateTransform();
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

/*!
Set the orientation as euler angles (degrees)
*/
void SGCTTracker::setOrientation(float xRot, float yRot, float zRot) {
    //create rotation quaternion based on x, y, z rotations
    glm::quat rotQuat;
    rotQuat = glm::rotate(rotQuat, glm::radians(xRot), glm::vec3(1.f, 0.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(yRot), glm::vec3(0.f, 1.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(zRot), glm::vec3(0.f, 0.f, 1.f));
    
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);

    //create inverse rotation matrix
    mOrientation = glm::inverse(glm::mat4_cast(rotQuat));

    calculateTransform();
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

/*!
Set the orientation as a quaternion
*/
void SGCTTracker::setOrientation(float w, float x, float y, float z) {
    glm::quat rotQuat(w, x, y, z);
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    
    //create inverse rotation matrix
    mOrientation = glm::inverse(glm::mat4_cast(rotQuat));

    calculateTransform();
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

void SGCTTracker::setOffset(float x, float y, float z) {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    mOffset = glm::vec3(x, y, z);
    calculateTransform();
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

void SGCTTracker::setScale(double scaleVal) {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    if (scaleVal > 0.0) {
        mScale = scaleVal;
    }
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

/*
Set the tracker system transform matrix\n
worldTransform = (trackerTransform * sensorMat) * deviceTransformMat
*/
void SGCTTracker::setTransform(glm::mat4 mat) {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    mXform = std::move(mat);
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

void SGCTTracker::calculateTransform() {
    //create offset translation matrix
    glm::mat4 transMat = glm::translate(glm::mat4(1.f), mOffset);
    
    //calculate transform
    mXform = transMat * mOrientation;
}

glm::mat4 SGCTTracker::getTransform() { 
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    glm::mat4 tmpMat = mXform;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpMat;
}

double SGCTTracker::getScale() {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    double tmpD = mScale;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpD;
}

size_t SGCTTracker::getNumberOfDevices() {
    return mTrackingDevices.size();
}

const std::string& SGCTTracker::getName() {
    return mName;
}

} // namespace sgct
