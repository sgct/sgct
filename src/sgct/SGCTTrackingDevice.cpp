/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/SGCTTrackingDevice.h>

#include <sgct/ClusterManager.h>
#include <sgct/Engine.h>
#include <sgct/MessageHandler.h>
#include <sgct/SGCTMutexManager.h>
#include <sgct/SGCTTracker.h>
#include <glm/gtc/matrix_transform.hpp>

namespace sgct {

/*!
Constructor
*/
SGCTTrackingDevice::SGCTTrackingDevice(size_t parentIndex, std::string name)
    : mName(std::move(name))
    , mParentIndex(parentIndex)
{}

/*!
Destructor
*/
SGCTTrackingDevice::~SGCTTrackingDevice() {
    mEnabled = false;

    if (mButtons != nullptr) {
        delete[] mButtons;
        mButtons = nullptr;
    }

    if (mButtonTime != nullptr) {
        delete[] mButtonTime;
        mButtonTime = nullptr;
    }

    if (mAxes != nullptr) {
        delete[] mAxes;
        mAxes = nullptr;
    }
}

/*!
Set if this device is enabled or not
*/
void SGCTTrackingDevice::setEnabled(bool state) {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    mEnabled = state;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

/*!
Set the id for this sensor
*/
void SGCTTrackingDevice::setSensorId(int id) {
    mSensorId = id;
}

/*!
Set the number of digital buttons
*/
void SGCTTrackingDevice::setNumberOfButtons(size_t numOfButtons) {
    if (mButtons != nullptr) {
        delete[] mButtons;
        mButtons = nullptr;
    }

    if (mButtonTime != nullptr) {
        delete[] mButtonTime;
        mButtonTime = nullptr;
    }

    //double buffered
    mButtons = new bool[numOfButtons * 2];
    mButtonTime = new double[numOfButtons * 2];

    mNumberOfButtons = numOfButtons;
    for (size_t i = 0; i < mNumberOfButtons; i++) {
        mButtons[i] = false;
        mButtonTime[i] = 0.0;
    }
}

/*!
Set the number of analog axes
*/
void SGCTTrackingDevice::setNumberOfAxes(size_t numOfAxes) {
    //clear
    if (mAxes != nullptr) {
        delete[] mAxes;
        mAxes = nullptr;
    }

    //double buffered
    mAxes = new double[numOfAxes * 2];
    mNumberOfAxes = numOfAxes;
    for (size_t i = 0; i < mNumberOfAxes; i++) {
        mAxes[i] = 0.0;
    }
}

void SGCTTrackingDevice::setSensorTransform(glm::dvec3 vec, glm::dquat rot) {
    sgct_core::ClusterManager& cm = *sgct_core::ClusterManager::instance();
    SGCTTracker* parent = cm.getTrackingManagerPtr().getTrackerPtr(mParentIndex);

    if (parent == nullptr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "SGCTTrackingDevice: Error, can't get handle to tracker for device '%s'!\n",
            mName.c_str()
        );
        return;
    }
    
    glm::mat4 systemTransformMatrix = parent->getTransform();
    //convert from double to float
    glm::quat sensorRot(
        static_cast<float>(rot.w),
        static_cast<float>(rot.x),
        static_cast<float>(rot.y),
        static_cast<float>(rot.z)
    );
    glm::vec3 sensorPos(vec);

    //create matrixes
    glm::mat4 sensorTransMat = glm::translate(glm::mat4(1.f), sensorPos);
    glm::mat4 sensorRotMat(glm::mat4_cast(sensorRot));

    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    
    //swap
    mSensorRotation[PREVIOUS] = mSensorRotation[CURRENT];
    mSensorRotation[CURRENT] = rot;

    mSensorPos[PREVIOUS] = mSensorPos[CURRENT];
    mSensorPos[CURRENT] = vec;

    mWorldTransform[PREVIOUS] = mWorldTransform[CURRENT];
    mWorldTransform[CURRENT] = systemTransformMatrix * sensorTransMat *
                               sensorRotMat * mDeviceTransformMatrix;

    /*glm::mat4 post_rot = glm::rotate(glm::mat4(1.0f),
        1.66745f,
        glm::vec3(0.928146f, 0.241998f, -0.282811f));

    glm::mat4 pre_rot = glm::rotate(glm::mat4(1.0f),
        2.09439f,
        glm::vec3(-0.57735f, -0.57735f, 0.57735f));

    glm::mat4 worldSensorRot = post_rot * glm::mat4_cast(sensorRot) * pre_rot;
    glm::vec4 worldSensorPos = glm::transpose(systemTransformMatrix) * glm::vec4( sensorPos, 1.0f);
    mWorldTransform[CURRENT] = glm::translate(glm::mat4(1.0f), glm::vec3(worldSensorPos)) * worldSensorRot;*/

    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    setTrackerTimeStamp();
}

void SGCTTrackingDevice::setButtonVal(bool val, size_t index) {
    if (index < mNumberOfButtons) {
        SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
        //swap
        mButtons[index + mNumberOfButtons] = mButtons[index];
        mButtons[index] = val;
        SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

        setButtonTimeStamp(index);
    }
}

void SGCTTrackingDevice::setAnalogVal(const double* array, size_t size) {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    for (size_t i = 0; i < size; i++) {
        if (i < mNumberOfAxes) {
            mAxes[i + mNumberOfAxes] = mAxes[i];
            mAxes[i] = array[i];
        }
    }
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    setAnalogTimeStamp();
}

/*!
Set the orientation euler angles (degrees) used to generate the orientation matrix\n
*/
void SGCTTrackingDevice::setOrientation(float xRot, float yRot, float zRot) {
    //create rotation quaternion based on x, y, z rotations
    glm::quat rotQuat;
    rotQuat = glm::rotate(rotQuat, glm::radians(xRot), glm::vec3(1.f, 0.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(yRot), glm::vec3(0.f, 1.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(zRot), glm::vec3(0.f, 0.f, 1.f));

    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    //create inverse rotation matrix
    mOrientation = rotQuat;

    calculateTransform();
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

/*!
Set the orientation quaternion used to generate the orientation matrix\n
*/
void SGCTTrackingDevice::setOrientation(float w, float x, float y, float z) {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    //create inverse rotation matrix
    mOrientation = glm::quat(w, x, y, z);

    calculateTransform();
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

/*!
Set the orientation quaternion used to generate the orientation matrix\n
*/
void SGCTTrackingDevice::setOrientation(glm::quat q) {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    //create inverse rotation matrix
    mOrientation = std::move(q);

    calculateTransform();
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

/*!
Set the offset vector used to generate the offset matrix\n
*/
void SGCTTrackingDevice::setOffset(float x, float y, float z) {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    mOffset = glm::vec3(x, y, z);
    calculateTransform();
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

/*!
Set the device transform matrix\n
*/
void SGCTTrackingDevice::setTransform(glm::mat4 mat) {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    mDeviceTransformMatrix = std::move(mat);
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

const std::string& SGCTTrackingDevice::getName() const {
    return mName;
}

size_t SGCTTrackingDevice::getNumberOfButtons() const {
    return mNumberOfButtons;
}

size_t SGCTTrackingDevice::getNumberOfAxes() const {
    return mNumberOfAxes;
}

void SGCTTrackingDevice::calculateTransform() {
    //create offset translation matrix
    glm::mat4 transMat = glm::translate(glm::mat4(1.f), mOffset);
    //calculate transform
    mDeviceTransformMatrix = transMat * glm::mat4_cast(mOrientation);
}

/*!
\returns the id of this device/sensor
*/
int SGCTTrackingDevice::getSensorId() {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get sensor id...\n");
#endif
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    int tmpVal = mSensorId;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpVal;
}

/*!
\returns a digital value from array
*/
bool SGCTTrackingDevice::getButton(size_t index, DataLocation i) {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get button from array...\n");
#endif
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    bool tmpVal = index < mNumberOfButtons ? mButtons[index + mNumberOfButtons * i] : false;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpVal;
}

/*!
\returns an analog value from array
*/
double SGCTTrackingDevice::getAnalog(size_t index, DataLocation i) {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get analog value...\n");
#endif
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    double tmpVal = index < mNumberOfAxes ? mAxes[index + mNumberOfAxes * i] : 0.0;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpVal;
}

/*!
\returns the sensor's position in world coordinates
*/
glm::vec3 SGCTTrackingDevice::getPosition(DataLocation i) {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get position...\n");
#endif
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    const glm::mat4& matRef = mWorldTransform[i];
    glm::vec3 tmpVal = glm::vec3(
        matRef[3][0],
        matRef[3][1],
        matRef[3][2]
    );
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpVal;
}

/*!
\returns the sensor's rotation as as euler angles in world coordinates
*/
glm::vec3 SGCTTrackingDevice::getEulerAngles(DataLocation i) {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get euler angles");
#endif
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    glm::vec3 tmpVal = glm::eulerAngles(glm::quat_cast(mWorldTransform[i]));
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpVal;
}

/*!
\returns the sensor's rotation as a quaternion in world coordinates
*/
glm::quat SGCTTrackingDevice::getRotation(DataLocation i) {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    glm::quat tmpQuat = glm::quat_cast(mWorldTransform[i]);
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpQuat;
}

/*!
\returns the sensor's transform matrix in world coordinates
*/
glm::mat4 SGCTTrackingDevice::getWorldTransform(DataLocation i) {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get transform matrix...\n");
#endif
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    glm::mat4 tmpMat = mWorldTransform[i];
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpMat;
}

/*!
\returns the raw sensor rotation quaternion
*/
glm::dquat SGCTTrackingDevice::getSensorRotation(DataLocation i) {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get sensor quaternion...\n");
#endif
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    glm::dquat tmpQuat = mSensorRotation[i];
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpQuat;
}

/*!
\returns the raw sensor position vector
*/
glm::dvec3 SGCTTrackingDevice::getSensorPosition(DataLocation i) {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get sensor position vector...\n");
#endif
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    glm::dvec3 tmpVec = mSensorPos[i];
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpVec;
}

bool SGCTTrackingDevice::isEnabled() {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Is device enabled...\n");
#endif
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex );
    bool tmpVal = mEnabled;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex );

    return tmpVal;
}

bool SGCTTrackingDevice::hasSensor() {
    return mSensorId != -1;
}

bool SGCTTrackingDevice::hasButtons() {
    return mNumberOfButtons > 0;
}

bool SGCTTrackingDevice::hasAnalogs() {
    return mNumberOfAxes > 0;
}

void SGCTTrackingDevice::setTrackerTimeStamp() {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    //swap
    mTrackerTime[1] = mTrackerTime[0];
    mTrackerTime[0] = Engine::getTime();
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

void SGCTTrackingDevice::setAnalogTimeStamp() {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    //swap
    mAnalogTime[1] = mAnalogTime[0];
    mAnalogTime[0] = Engine::getTime();
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

void SGCTTrackingDevice::setButtonTimeStamp(size_t index) {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    //swap
    mButtonTime[index + mNumberOfButtons] = mButtonTime[index];
    mButtonTime[index] = Engine::getTime();
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

double SGCTTrackingDevice::getTrackerTimeStamp(DataLocation i) {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get device tracker time stamp...\n");
#endif
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex );
    double tmpVal = mTrackerTime[i];
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex );

    return tmpVal;
}

double SGCTTrackingDevice::getAnalogTimeStamp(DataLocation i) {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get device analog time stamp...\n");
#endif
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    double tmpVal = mAnalogTime[i];
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    return tmpVal;
}

double SGCTTrackingDevice::getButtonTimeStamp(size_t index, DataLocation i) {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get device button time stamp...\n");
#endif

    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex );
    double tmpVal = mButtonTime[index + mNumberOfButtons * i];
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex );

    return tmpVal;
}

double SGCTTrackingDevice::getTrackerDeltaTime() {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get device tracker delta time...\n");
#endif

    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    double tmpVal = mTrackerTime[0] - mTrackerTime[1];
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    return tmpVal;
}

double SGCTTrackingDevice::getAnalogDeltaTime() {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get device analog delta time...\n");
#endif

    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    double tmpVal = mAnalogTime[0] - mAnalogTime[1];
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    return tmpVal;
}

double SGCTTrackingDevice::getButtonDeltaTime(size_t index) {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get device button delta time...\n");
#endif

    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    double tmpVal = mButtonTime[index] - mButtonTime[index + mNumberOfButtons];
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    return tmpVal;
}

} // namespace sgct
