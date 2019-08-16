/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__TRACKING_DEVICE__H__
#define __SGCT__TRACKING_DEVICE__H__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

namespace sgct {

/*!
Helper class that holds tracking device/sensor data
*/
class SGCTTrackingDevice {
public:
    enum DataLocation { CURRENT = 0, PREVIOUS };

    SGCTTrackingDevice(size_t parentIndex, std::string name);
    ~SGCTTrackingDevice();

    void setEnabled(bool state);
    void setSensorId(int id);
    void setNumberOfButtons(int numOfButtons);
    void setNumberOfAxes(int numOfAxes);
    void setSensorTransform(glm::dvec3 vec, glm::dquat rot );
    void setButtonVal(bool val, size_t index);
    void setAnalogVal(const double* array, size_t size);
    void setOrientation(float xRot, float yRot, float zRot);
    void setOrientation(float w, float x, float y, float z);
    void setOrientation(glm::quat q);
    void setOffset(float x, float y, float z);
    void setTransform(glm::mat4 mat);

    const std::string& getName() const;
    size_t getNumberOfButtons() const;
    size_t getNumberOfAxes() const;
    bool getButton(size_t index, DataLocation i = CURRENT);
    double getAnalog(size_t index, DataLocation i = CURRENT);
    bool isEnabled();
    bool hasSensor();
    bool hasButtons();
    bool hasAnalogs();
    int getSensorId();

    glm::vec3 getPosition(DataLocation i = CURRENT);
    glm::vec3 getEulerAngles(DataLocation i = CURRENT);
    glm::quat getRotation(DataLocation i = CURRENT);
    glm::mat4 getWorldTransform(DataLocation i = CURRENT);
    glm::dquat getSensorRotation(DataLocation i = CURRENT);
    glm::dvec3 getSensorPosition(DataLocation i = CURRENT);

    double getTrackerTimeStamp(DataLocation i = CURRENT);
    double getAnalogTimeStamp(DataLocation i = CURRENT);
    double getButtonTimeStamp(size_t index, DataLocation i = CURRENT);

    double getTrackerDeltaTime();
    double getAnalogDeltaTime();
    double getButtonDeltaTime(size_t index);

private:
    void calculateTransform();
    void setTrackerTimeStamp();
    void setAnalogTimeStamp();
    void setButtonTimeStamp(size_t index);

private:
    bool mEnabled = true;
    std::string mName;
    size_t mParentIndex; //the index of parent SGCTTracker
    size_t mNumberOfButtons = 0;
    size_t mNumberOfAxes = 0;
    int mSensorId = -1;

    glm::mat4 mDeviceTransformMatrix = glm::mat4(1.f);
    glm::mat4 mWorldTransform[2] = { glm::mat4(1.f), glm::mat4(1.f) };
    glm::dquat mSensorRotation[2] = {
        glm::dquat(0.0, 0.0, 0.0, 0.0), glm::dquat(0.0, 0.0, 0.0, 0.0)
    };
    glm::dvec3 mSensorPos[2] = { glm::dvec3(0.0), glm::dvec3(0.0) };
    glm::quat mOrientation;
    glm::vec3 mOffset;

    double mTrackerTime[2] = { 0.0, 0.0 };
    double mAnalogTime[2] = { 0.0, 0.0 };
    double* mButtonTime = nullptr;
    bool* mButtons = nullptr;
    double* mAxes = nullptr;
};

} // namespace sgct

#endif // __SGCT__TRACKING_DEVICE__H__
