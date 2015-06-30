/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_TRACKER_H_
#define _SGCT_TRACKER_H_

#include <vector>
#include "SGCTTrackingDevice.h"

namespace sgct
{

/*!
Class that manages a tracking system's properties and devices/sensors
*/
class SGCTTracker
{
public:
	SGCTTracker(std::string name);
	~SGCTTracker();
	void setEnabled(bool state);
	void addDevice(std::string name, size_t index);

	SGCTTrackingDevice * getLastDevicePtr();
	SGCTTrackingDevice * getDevicePtr(size_t index);
	SGCTTrackingDevice * getDevicePtr(const char * name);
	SGCTTrackingDevice * getDevicePtrBySensorId(int id);

	void setOrientation(glm::quat q);
	void setOrientation(float xRot, float yRot, float zRot);
	void setOrientation(float w, float x, float y, float z);
	void setOffset(float x, float y, float z);
	void setScale(double scaleVal);
	void setTransform(glm::mat4 mat);

	glm::mat4 getTransform();
	double getScale();

	inline size_t getNumberOfDevices() { return mTrackingDevices.size(); }
	inline const std::string & getName() { return mName; }

private:
	void calculateTransform();

private:
	std::vector<SGCTTrackingDevice *> mTrackingDevices;
	std::string mName;

	double mScale;
	glm::mat4 mXform;
	glm::mat4 mOrientation;
	glm::vec3 mOffset;
};

}

#endif
