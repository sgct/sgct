/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _SGCT_TRACKING_DEVICE_H_
#define _SGCT_TRACKING_DEVICE_H_

#include <string>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
//#include <glm/gtx/quaternion.hpp>

typedef void * GLFWmutex;

namespace sgct
{

/*!
Helper class that holds tracking device/sensor data
*/
class SGCTTrackingDevice
{
public:
	enum DataLoc { CURRENT = 0, PREVIOUS };

	SGCTTrackingDevice(size_t parentIndex, std::string name);
	~SGCTTrackingDevice();

	void setEnabled(bool state);
	void setSensorId(int id);
	void setNumberOfButtons(size_t numOfButtons);
	void setNumberOfAxes(size_t numOfAxes);
	void setSensorTransform( glm::dvec3 vec, glm::dquat rot );
	void setButtonVal(const bool val, size_t index);
	void setAnalogVal(const double * array, size_t size);
	void setOrientation(float xRot, float yRot, float zRot);
	void setOrientation(float w, float x, float y, float z);
	void setOrientation(glm::quat q);
	void setOffset(float x, float y, float z);
	void setTransform(glm::mat4 mat);

	inline const std::string & getName() { return mName; }
	inline size_t getNumberOfButtons() { return mNumberOfButtons; }
	inline size_t getNumberOfAxes() { return mNumberOfAxes; }
	bool getButton(size_t index, DataLoc i = CURRENT);
	double getAnalog(size_t index, DataLoc i = CURRENT);
	bool isEnabled();
	inline bool hasSensor() { return mSensorId != -1; }
	inline bool hasButtons() { return mNumberOfButtons > 0; }
	inline bool hasAnalogs() { return mNumberOfAxes > 0; }
	int getSensorId();

	glm::vec3 getPosition(DataLoc i = CURRENT);
	glm::vec3 getEulerAngles(DataLoc i = CURRENT);
	glm::quat getRotation(DataLoc i = CURRENT);
	glm::mat4 getWorldTransform(DataLoc i = CURRENT);
	glm::dquat getSensorRotation(DataLoc i = CURRENT);
	glm::dvec3 getSensorPosition(DataLoc i = CURRENT);

	double getTrackerTimeStamp(DataLoc i = CURRENT);
	double getAnalogTimeStamp(DataLoc i = CURRENT);
	double getButtonTimeStamp(size_t index, DataLoc i = CURRENT);

	double getTrackerDeltaTime();
	double getAnalogDeltaTime();
	double getButtonDeltaTime(size_t index);

private:
	void calculateTransform();
	void setTrackerTimeStamp();
	void setAnalogTimeStamp();
	void setButtonTimeStamp(size_t index);

private:
	bool mEnabled;
	std::string mName;
	size_t mParentIndex; //the index of parent SGCTTracker
	size_t mNumberOfButtons;
	size_t mNumberOfAxes;
	int mSensorId;

	glm::mat4 mDeviceTransformMatrix;
	glm::mat4 mWorldTransform[2];
	glm::dquat mSensorRotation[2];
	glm::dvec3 mSensorPos[2];
	glm::quat mOrientation;
	glm::vec3 mOffset;

	double mTrackerTime[2];
	double mAnalogTime[2];
	double * mButtonTime;
	bool * mButtons;
	double * mAxes;
};

}

#endif
