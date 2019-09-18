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
#include <vector>

namespace sgct {

/**
 * Helper class that holds tracking device/sensor data
*/
class TrackingDevice {
public:
    /// Constructor
    TrackingDevice(size_t parentIndex, std::string name);
    
    /// Set if this device is enabled or not
    void setEnabled(bool state);

    /// Set the id for this sensor
    void setSensorId(int id);

    /// Set the number of digital buttons
    void setNumberOfButtons(int numOfButtons);

    /// Set the number of analog axes
    void setNumberOfAxes(int numOfAxes);
    void setSensorTransform(glm::dvec3 vec, glm::dquat rot);
    void setButtonVal(bool val, int index);
    void setAnalogVal(const double* array, int size);

    /// Set the orientation euler angles (degrees) used to generate the orientation matrix
    void setOrientation(float xRot, float yRot, float zRot);

    /// Set the orientation quaternion used to generate the orientation matrix
    void setOrientation(glm::quat q);

    /// Set the offset vector used to generate the offset matrix
    void setOffset(glm::vec3 offset);

    /// Set the device transform matrix
    void setTransform(glm::mat4 mat);

    const std::string& getName() const;
    int getNumberOfButtons() const;
    int getNumberOfAxes() const;

    /// \return a digital value from array
    bool getButton(int index) const;
    
    /// \return a digital value from array
    bool getButtonPrevious(int index) const;

    /// \return an analog value from array
    double getAnalog(int index) const;

    /// \return an analog value from array
    double getAnalogPrevious(int index) const;
    
    bool isEnabled() const;
    bool hasSensor() const;
    bool hasButtons() const;
    bool hasAnalogs() const;

    /// \return the id of this device/sensor
    int getSensorId();

    /// \return the sensor's position in world coordinates
    glm::vec3 getPosition() const;

    /// \return the sensor's position in world coordinates
    glm::vec3 getPreviousPosition() const;

    /// \return the sensor's rotation as as euler angles in world coordinates
    glm::vec3 getEulerAngles() const;

    /// \return the sensor's rotation as as euler angles in world coordinates
    glm::vec3 getEulerAnglesPrevious() const;

    /// \return the sensor's rotation as a quaternion in world coordinates
    glm::quat getRotation() const;

    /// \return the sensor's rotation as a quaternion in world coordinates
    glm::quat getRotationPrevious() const;

    /// \return the sensor's transform matrix in world coordinates
    glm::mat4 getWorldTransform() const;

    /// \return the sensor's transform matrix in world coordinates
    glm::mat4 getWorldTransformPrevious() const;

    /// \return the raw sensor rotation quaternion
    glm::dquat getSensorRotation() const;

    /// \return the raw sensor rotation quaternion
    glm::dquat getSensorRotationPrevious() const;

    /// \return the raw sensor position vector
    glm::dvec3 getSensorPosition() const;

    /// \return the raw sensor position vector
    glm::dvec3 getSensorPositionPrevious() const;

    double getTrackerTimeStamp();
    double getTrackerTimeStampPrevious();

    double getAnalogTimeStamp() const;
    double getAnalogTimeStampPrevious() const;
    double getButtonTimeStamp(size_t index) const;
    double getButtonTimeStampPrevious(size_t index) const;

    double getTrackerDeltaTime() const;
    double getAnalogDeltaTime() const;
    double getButtonDeltaTime(size_t index) const;

private:
    void calculateTransform();
    void setTrackerTimeStamp();
    void setAnalogTimeStamp();
    void setButtonTimeStamp(size_t index);

    bool mEnabled = true;
    std::string mName;
    size_t mParentIndex; //the index of parent Tracker
    int mNumberOfButtons = 0;
    int mNumberOfAxes = 0;
    int mSensorId = -1;

    glm::mat4 mDeviceTransform = glm::mat4(1.f);

    glm::mat4 mWorldTransform = glm::mat4(1.f);
    glm::mat4 mWorldTransformPrevious = glm::mat4(1.f);

    glm::dquat mSensorRotation = glm::dquat(0.0, 0.0, 0.0, 0.0);
    glm::dquat mSensorRotationPrevious = glm::dquat(0.0, 0.0, 0.0, 0.0);

    glm::dvec3 mSensorPos = glm::dvec3(0.0);
    glm::dvec3 mSensorPosPrevious = glm::dvec3(0.0);
    glm::quat mOrientation;
    glm::vec3 mOffset;

    double mTrackerTime = 0.0;
    double mTrackerTimePrevious = 0.0;

    double mAnalogTime = 0.0;
    double mAnalogTimePrevious = 0.0;

    std::vector<double> mButtonTime;
    std::vector<double> mButtonTimePrevious;
    std::vector<bool> mButtons;
    std::vector<bool> mButtonsPrevious;
    std::vector<double> mAxes;
    std::vector<double> mAxesPrevious;
};

} // namespace sgct

#endif // __SGCT__TRACKING_DEVICE__H__
