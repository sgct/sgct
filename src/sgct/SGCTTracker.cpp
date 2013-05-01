/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/SGCTTracker.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/Engine.h"

extern GLFWmutex gTrackingMutex;

sgct::SGCTTracker::SGCTTracker(std::string name)
{
	mName.assign(name);

	mXform = glm::dmat4(1.0);
	mOffset = glm::dvec3(0.0);
	mQuatTransform = glm::dvec4(1.0);

	mScale = 1.0;
}

sgct::SGCTTracker::~SGCTTracker()
{
	for(size_t i=0; i<mTrackingDevices.size(); i++)
	{
		if( mTrackingDevices[i] != NULL )
		{
			delete mTrackingDevices[i];
			mTrackingDevices[i] = NULL;
		}
	}

	mTrackingDevices.clear();
}

void sgct::SGCTTracker::setEnabled(bool state)
{
	for(size_t i=0; i<mTrackingDevices.size(); i++)
	{
		mTrackingDevices[i]->setEnabled( state );
	}
}

void sgct::SGCTTracker::addDevice(std::string name, size_t index)
{
	SGCTTrackingDevice * td = new SGCTTrackingDevice( index, name );

	mTrackingDevices.push_back( td );

	MessageHandler::Instance()->print("%s: Adding device '%s'...\n", mName.c_str(), name.c_str());
}

void sgct::SGCTTracker::addSensorToDevice(const char * address, int id)
{
	if(mTrackingDevices.empty())
		return;
	fprintf(stderr, "Device: %d\n", id);
	mTrackingDevices.back()->setSensorId( id );
}

void sgct::SGCTTracker::addButtonsToDevice(const char * address, size_t numOfButtons)
{
	if(mTrackingDevices.empty())
		return;

	mTrackingDevices.back()->setNumberOfButtons( numOfButtons );
}

void sgct::SGCTTracker::addAnalogsToDevice(const char * address, size_t numOfAxes)
{
	if(mTrackingDevices.empty())
		return;

	mTrackingDevices.back()->setNumberOfAxes( numOfAxes );
}

sgct::SGCTTrackingDevice * sgct::SGCTTracker::getLastDevicePtr()
{
	return mTrackingDevices.size() > 0 ? mTrackingDevices.back() : NULL;
}

sgct::SGCTTrackingDevice * sgct::SGCTTracker::getDevicePtr(size_t index)
{
	return index < mTrackingDevices.size() ? mTrackingDevices[index] : NULL;
}

sgct::SGCTTrackingDevice * sgct::SGCTTracker::getDevicePtr(const char * name)
{
	for(size_t i=0; i<mTrackingDevices.size(); i++)
	{
		if( strcmp(name, mTrackingDevices[i]->getName().c_str()) == 0 )
			return mTrackingDevices[i];
	}

	//if not found
	return NULL;
}

sgct::SGCTTrackingDevice * sgct::SGCTTracker::getDevicePtrBySensorId(int id)
{
	for(size_t i=0; i<mTrackingDevices.size(); i++)
	{
		if( mTrackingDevices[i]->getSensorId() == id )
			return mTrackingDevices[i];
	}

	return NULL;
}

void sgct::SGCTTracker::setOrientation(double xRot, double yRot, double zRot)
{
	SGCTMutexManager::Instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		//create rotation quaternion based on x, y, z rotations
		glm::dquat rotQuat;
		rotQuat = glm::rotate( rotQuat, xRot, glm::dvec3(1.0, 0.0, 0.0) );
		rotQuat = glm::rotate( rotQuat, yRot, glm::dvec3(0.0, 1.0, 0.0) );
		rotQuat = glm::rotate( rotQuat, zRot, glm::dvec3(0.0, 0.0, 1.0) );

		//create inverse rotation matrix
		mOrientation = glm::inverse( glm::mat4_cast(rotQuat) );

		calculateTransform();
	SGCTMutexManager::Instance()->unlockMutex( SGCTMutexManager::TrackingMutex );
}

void sgct::SGCTTracker::setOffset(double x, double y, double z)
{
	SGCTMutexManager::Instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		mOffset[0] = x;
		mOffset[1] = y;
		mOffset[2] = z;

		calculateTransform();
	SGCTMutexManager::Instance()->unlockMutex( SGCTMutexManager::TrackingMutex );
}

void sgct::SGCTTracker::setScale(double scaleVal)
{
	SGCTMutexManager::Instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		if( scaleVal > 0.0 )
			mScale = scaleVal;
	SGCTMutexManager::Instance()->unlockMutex( SGCTMutexManager::TrackingMutex );
}

void sgct::SGCTTracker::calculateTransform()
{
	//create offset translation matrix
	glm::dmat4 transMat = glm::translate( glm::dmat4(1.0), mOffset );
	//calculate transform
	mXform = transMat * mOrientation;
}
