/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_TRACKER_H_
#define _SGCT_TRACKER_H_

#include <vector>
#include "SGCTTrackingDevice.h"

namespace sgct
{

class SGCTTracker
{
public:
	SGCTTracker(std::string name);
	~SGCTTracker();
	void setEnabled(bool state);
	void addDevice(std::string name, size_t index);
	void addSensorToDevice(const char * address, int id);
	void addButtonsToDevice(const char * address, size_t numOfButtons);
	void addAnalogsToDevice(const char * address, size_t numOfAxes);

	SGCTTrackingDevice * getLastDevicePtr();
	SGCTTrackingDevice * getDevicePtr(size_t index);
	SGCTTrackingDevice * getDevicePtr(const char * name);
	SGCTTrackingDevice * getDevicePtrBySensorId(int id);

	void setOrientation(double xRot, double yRot, double zRot);
	void setOffset(double x, double y, double z);
	void setScale(double scaleVal);

	inline glm::dmat4 getTransform() { return mXform; }
	inline double getScale() { return mScale; }
	inline const glm::dvec4 & getQuatTransform() { return mQuatTransform; }

	inline size_t getNumberOfDevices() { return mTrackingDevices.size(); }
	inline const std::string & getName() { return mName; }

private:
	void calculateTransform();

private:
	std::vector<SGCTTrackingDevice *> mTrackingDevices;
	std::string mName;

	double mScale;
	glm::dmat4 mXform;
	glm::dmat4 mOrientation;
	glm::dvec3 mOffset;
	glm::dvec4 mQuatTransform;
};

}

#endif
