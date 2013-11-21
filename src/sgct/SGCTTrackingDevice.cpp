/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "../include/sgct/ClusterManager.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/SGCTTrackingDevice.h"
#include "../include/sgct/Engine.h"

extern GLFWmutex gTrackingMutex;

/*!
Constructor
*/
sgct::SGCTTrackingDevice::SGCTTrackingDevice(size_t parentIndex, std::string name)
{
	mEnabled = true;
	mName.assign( name );
	mParentIndex = parentIndex;
	mNumberOfButtons = 0;
	mNumberOfAxes = 0;

	mWorldTransform[0] = glm::dmat4(1.0);
	mWorldTransform[1] = glm::dmat4(1.0);
	mPostTransform = glm::dmat4(1.0);

	mButtons = NULL;
	mAxes = NULL;
	mButtonTime = NULL;
	mTrackerTime[0] = 0.0;
	mTrackerTime[1] = 0.0;
	mAnalogTime[0] = 0.0;
	mAnalogTime[1] = 0.0;
	mSensorId = -1;
}

/*!
Destructor
*/
sgct::SGCTTrackingDevice::~SGCTTrackingDevice()
{
	mEnabled = false;

	if( mButtons != NULL )
	{
		delete [] mButtons;
		mButtons = NULL;
	}

	if( mButtonTime != NULL )
	{
		delete [] mButtonTime;
		mButtonTime = NULL;
	}

	if( mAxes != NULL )
	{
		delete [] mAxes;
		mAxes = NULL;
	}
}

/*!
Set if this device is enabled or not
*/
void sgct::SGCTTrackingDevice::setEnabled(bool state)
{
	SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
	mEnabled = state;
	SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

/*!
Set the id for this sensor
*/
void sgct::SGCTTrackingDevice::setSensorId(int id)
{
	mSensorId = id;
}

/*!
Set the number of digital buttons
*/
void sgct::SGCTTrackingDevice::setNumberOfButtons(size_t numOfButtons)
{
	if (mButtons != NULL)
	{
		delete[] mButtons;
		mButtons = NULL;
	}

	if (mButtonTime != NULL)
	{
		delete[] mButtonTime;
		mButtonTime = NULL;
	}

	//double buffered
	mButtons = new bool[numOfButtons * 2];
	mButtonTime = new double[numOfButtons * 2];

	mNumberOfButtons = numOfButtons;
	for(size_t i=0; i<mNumberOfButtons; i++)
	{
		mButtons[i] = false;
		mButtonTime[i] = 0.0;
	}
}

/*!
Set the number of analog axes
*/
void sgct::SGCTTrackingDevice::setNumberOfAxes(size_t numOfAxes)
{
	//clear
	if (mAxes != NULL)
	{
		delete[] mAxes;
		mAxes = NULL;
	}

	//double buffered
	mAxes = new double[numOfAxes * 2];
	mNumberOfAxes = numOfAxes;
	for(size_t i=0; i<mNumberOfAxes; i++)
	{
		mAxes[i] = 0.0;
	}
}

void sgct::SGCTTrackingDevice::setSensorTransform( glm::dmat4 mat )
{
	sgct::SGCTTracker * parent = sgct_core::ClusterManager::instance()->getTrackingManagerPtr()->getTrackerPtr(mParentIndex);

	if (parent == NULL)
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "SGCTTrackingDevice: Error, can't get handle to tracker for device '%s'!\n", mName.c_str());
		return;
	}
	
	const glm::dmat4 & preTransform = parent->getTransform();

	SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    //swap
    mWorldTransform[PREVIOUS] = mWorldTransform[CURRENT];
    mWorldTransform[CURRENT] = (preTransform * mat) * mPostTransform;
	SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    setTrackerTimeStamp();
}

void sgct::SGCTTrackingDevice::setButtonVal(const bool val, size_t index)
{
	if( index < mNumberOfButtons )
	{
		SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
		//swap
        mButtons[index + mNumberOfButtons] = mButtons[index];
        mButtons[index] = val;

		setButtonTimeStamp( index );
		SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    }
}

void sgct::SGCTTrackingDevice::setAnalogVal(const double * array, size_t size)
{
	SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
	for (size_t i = 0; i < size; i++)
	{
		if (i < mNumberOfAxes)
		{
			mAxes[i + mNumberOfAxes] = mAxes[i];
			mAxes[i] = array[i];
		}
	}
	SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    setAnalogTimeStamp();
}

/*!
Set the orientation euler angles (degrees) used to generate the orientation matrix\n
transform = offsetMat * orientationMat
*/
void sgct::SGCTTrackingDevice::setOrientation(double xRot, double yRot, double zRot)
{
	//create rotation quaternion based on x, y, z rotations
	glm::dquat rotQuat;
	rotQuat = glm::rotate( rotQuat, xRot, glm::dvec3(1.0, 0.0, 0.0) );
	rotQuat = glm::rotate( rotQuat, yRot, glm::dvec3(0.0, 1.0, 0.0) );
	rotQuat = glm::rotate( rotQuat, zRot, glm::dvec3(0.0, 0.0, 1.0) );

	SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
	//create inverse rotation matrix
	mOrientation = glm::mat4_cast(rotQuat);

	calculateTransform();
	SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

/*!
Set the orientation quaternion used to generate the orientation matrix\n
transform = offsetMat * orientationMat
*/
void sgct::SGCTTrackingDevice::setOrientation(double w, double x, double y, double z)
{
	glm::dquat rotQuat(w, x, y, z);
	
	SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
	//create inverse rotation matrix
	mOrientation = glm::mat4_cast(rotQuat);

	calculateTransform();
	SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

/*!
Set the offset vector used to generate the offset matrix\n
transform = offsetMat * orientationMat
*/
void sgct::SGCTTrackingDevice::setOffset(double x, double y, double z)
{
	SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
	mOffset[0] = x;
	mOffset[1] = y;
	mOffset[2] = z;

	calculateTransform();
	SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

/*!
Set the device transform matrix\n
worldTransform = (trackerTransform * sensorMat) * deviceTransformMat
*/
void sgct::SGCTTrackingDevice::setTransform(glm::dmat4 mat)
{
	SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
	mPostTransform = mat;
	SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

void sgct::SGCTTrackingDevice::calculateTransform()
{
	//create offset translation matrix
	glm::dmat4 transMat = glm::translate( glm::dmat4(1.0), mOffset );
	//calculate transform
	mPostTransform = transMat * mOrientation;
}

int sgct::SGCTTrackingDevice::getSensorId()
{
	int tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get sensor...\n");
#endif
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		tmpVal = mSensorId;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );
	return tmpVal;
}

bool sgct::SGCTTrackingDevice::getButton(size_t index, DataLoc i)
{
	bool tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get button...\n");
#endif
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		tmpVal = index < mNumberOfButtons ? mButtons[index + mNumberOfButtons * i] : false;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );
	return tmpVal;
}

double sgct::SGCTTrackingDevice::getAnalog(size_t index, DataLoc i)
{
	double tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get analog array...\n");
#endif
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		tmpVal = index < mNumberOfAxes ? mAxes[index + mNumberOfAxes * i] : false;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );
	return tmpVal;
}

glm::dvec3 sgct::SGCTTrackingDevice::getPosition(DataLoc i)
{
	glm::dvec3 tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get position...\n");
#endif
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		glm::dmat4 & matRef = mWorldTransform[i];
		tmpVal[0] = matRef[3][0];
		tmpVal[1] = matRef[3][1];
		tmpVal[2] = matRef[3][2];
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );
	return tmpVal;
}

glm::dvec3 sgct::SGCTTrackingDevice::getEulerAngles(DataLoc i)
{
	glm::dvec3 tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get euler angles");
#endif
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		tmpVal = glm::eulerAngles( glm::quat_cast(mWorldTransform[i]) );
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );
	return tmpVal;
}

glm::dmat4 sgct::SGCTTrackingDevice::getTransform(DataLoc i)
{
	glm::dmat4 tmpMat;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get transform matrix...\n");
#endif
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		tmpMat = mWorldTransform[i];
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );
	return tmpMat;
}

bool sgct::SGCTTrackingDevice::isEnabled()
{
    bool tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Is device enabled...\n");
#endif
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		tmpVal = mEnabled;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );

    return tmpVal;
}

void sgct::SGCTTrackingDevice::setTrackerTimeStamp()
{
	SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
	//swap
	mTrackerTime[1] = mTrackerTime[0];
	mTrackerTime[0] = sgct::Engine::getTime();
	SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

void sgct::SGCTTrackingDevice::setAnalogTimeStamp()
{
	SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
	//swap
	mAnalogTime[1] = mAnalogTime[0];
	mAnalogTime[0] = sgct::Engine::getTime();
	SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

void sgct::SGCTTrackingDevice::setButtonTimeStamp(size_t index)
{
	SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
	//swap
	mButtonTime[index + mNumberOfButtons] = mButtonTime[index];
	mButtonTime[index] = sgct::Engine::getTime();
	SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

double sgct::SGCTTrackingDevice::getTrackerTimeStamp(DataLoc i)
{
	double tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get device tracker time stamp...\n");
#endif
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		tmpVal = mTrackerTime[i];
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );

	return tmpVal;
}

double sgct::SGCTTrackingDevice::getAnalogTimeStamp(DataLoc i)
{
	double tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get device analog time stamp...\n");
#endif
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		tmpVal = mAnalogTime[i];
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );

	return tmpVal;
}

double sgct::SGCTTrackingDevice::getButtonTimeStamp(size_t index, DataLoc i)
{
	double tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get device button time stamp...\n");
#endif

	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		tmpVal = mButtonTime[index + mNumberOfButtons * i];
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );

	return tmpVal;
}

double sgct::SGCTTrackingDevice::getTrackerDeltaTime()
{
	double tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get device tracker delta time...\n");
#endif

	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		tmpVal = mTrackerTime[0] - mTrackerTime[1];
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );

	return tmpVal;
}

double sgct::SGCTTrackingDevice::getAnalogDeltaTime()
{
	double tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get device analog delta time...\n");
#endif

	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		tmpVal = mAnalogTime[0] - mAnalogTime[1];
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );

	return tmpVal;
}

double sgct::SGCTTrackingDevice::getButtonDeltaTime(size_t index)
{
	double tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get device button delta time...\n");
#endif

	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		tmpVal = mButtonTime[index] - mButtonTime[index + mNumberOfButtons];
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );

	return tmpVal;
}
