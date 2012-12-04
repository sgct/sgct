/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_TRACKING_DEVICE_H_
#define _SGCT_TRACKING_DEVICE_H_

#include <string>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

typedef void * GLFWmutex;

namespace sgct
{

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
	void setSensorTransform( glm::dmat4 mat );
	void setButtonVal(const bool val, size_t index);
	void setAnalogVal(const double * array, size_t size);
	void setOrientation(double xRot, double yRot, double zRot);
	void setOffset(double x, double y, double z);

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

	glm::dvec3 getPosition(DataLoc i = CURRENT);
	glm::dvec3 getEulerAngles(DataLoc i = CURRENT);
	glm::dmat4 getTransformMat(DataLoc i = CURRENT);

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

	glm::dmat4 mPostTransform;
	glm::dmat4 mWorldTransform[2];
	glm::dmat4 mOrientation;
	glm::dvec3 mOffset;

	double mTrackerTime[2];
	double mAnalogTime[2];
	double * mButtonTime;
	bool * mButtons;
	double * mAxes;
};

}

#endif
