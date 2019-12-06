/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__TRACKINGDEVICE__H__
#define __SGCT__TRACKINGDEVICE__H__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

namespace sgct {

/// Helper class that holds tracking device/sensor data
class TrackingDevice {
public:
    /// Constructor
    TrackingDevice(int parentIndex, std::string name);
    
    /// Set if this device is enabled or not
    void setEnabled(bool state);

    /// Set the id for this sensor
    void setSensorId(int id);

    /// Set the number of digital buttons
    void setNumberOfButtons(int numOfButtons);

    /// Set the number of analog axes
    void setNumberOfAxes(int numOfAxes);
    void setSensorTransform(glm::vec3 vec, glm::quat rot);
    void setButtonValue(bool val, int index);
    void setAnalogValue(const double* array, int size);

    /// Set the orientation euler angles (degrees) used to generate the orientation matrix
    void setOrientation(float xRot, float yRot, float zRot);

    /// Set the orientation quaternion used to generate the orientation matrix
    void setOrientation(glm::quat q);

    /// Set the offset vector used to generate the offset matrix
    void setOffset(glm::vec3 offset);

    /// Set the device transform matrix
    void setTransform(glm::mat4 mat);

    const std::string& name() const;
    int numberOfButtons() const;
    int numberOfAxes() const;

    /// \return a digital value from array
    bool button(int index) const;
    
    /// \return a digital value from array
    bool buttonPrevious(int index) const;

    /// \return an analog value from array
    double analog(int index) const;

    /// \return an analog value from array
    double analogPrevious(int index) const;
    
    bool isEnabled() const;
    bool hasSensor() const;
    bool hasButtons() const;
    bool hasAnalogs() const;

    /// \return the id of this device/sensor
    int sensorId();

    /// \return the sensor's position in world coordinates
    glm::vec3 position() const;

    /// \return the sensor's position in world coordinates
    glm::vec3 previousPosition() const;

    /// \return the sensor's rotation as as euler angles in world coordinates
    glm::vec3 eulerAngles() const;

    /// \return the sensor's rotation as as euler angles in world coordinates
    glm::vec3 eulerAnglesPrevious() const;

    /// \return the sensor's rotation as a quaternion in world coordinates
    glm::quat rotation() const;

    /// \return the sensor's rotation as a quaternion in world coordinates
    glm::quat rotationPrevious() const;

    /// \return the sensor's transform matrix in world coordinates
    glm::mat4 worldTransform() const;

    /// \return the sensor's transform matrix in world coordinates
    glm::mat4 worldTransformPrevious() const;

    /// \return the raw sensor rotation quaternion
    glm::dquat sensorRotation() const;

    /// \return the raw sensor rotation quaternion
    glm::dquat sensorRotationPrevious() const;

    /// \return the raw sensor position vector
    glm::dvec3 sensorPosition() const;

    /// \return the raw sensor position vector
    glm::dvec3 sensorPositionPrevious() const;

    double trackerTimeStamp();
    double trackerTimeStampPrevious();

    double analogTimeStamp() const;
    double analogTimeStampPrevious() const;
    double buttonTimeStamp(int index) const;
    double buttonTimeStampPrevious(int index) const;

    double trackerDeltaTime() const;
    double analogDeltaTime() const;
    double buttonDeltaTime(int index) const;

private:
    void calculateTransform();
    void setTrackerTimeStamp();
    void setAnalogTimeStamp();
    void setButtonTimeStamp(int index);

    bool _isEnabled = true;
    const std::string _name;
    const int _parentIndex; // the index of parent Tracker
    int _nButtons = 0;
    int _nAxes = 0;
    int _sensorId = -1;

    glm::mat4 _deviceTransform = glm::mat4(1.f);

    glm::mat4 _worldTransform = glm::mat4(1.f);
    glm::mat4 _worldTransformPrevious = glm::mat4(1.f);

    glm::dquat _sensorRotation = glm::dquat(0.0, 0.0, 0.0, 0.0);
    glm::dquat _sensorRotationPrevious = glm::dquat(0.0, 0.0, 0.0, 0.0);

    glm::dvec3 _sensorPos = glm::dvec3(0.0);
    glm::dvec3 _sensorPosPrevious = glm::dvec3(0.0);
    glm::quat _orientation = glm::quat(1.f, 0.f, 0.f, 0.f);
    glm::vec3 _offset = glm::vec3(0.f);

    double _trackerTime = 0.0;
    double _trackerTimePrevious = 0.0;

    double _analogTime = 0.0;
    double _analogTimePrevious = 0.0;

    std::vector<double> _buttonTime;
    std::vector<double> _buttonTimePrevious;
    std::vector<bool> _buttons;
    std::vector<bool> _buttonsPrevious;
    std::vector<double> _axes;
    std::vector<double> _axesPrevious;
};

} // namespace sgct

#endif // __SGCT__TRACKINGDEVICE__H__
