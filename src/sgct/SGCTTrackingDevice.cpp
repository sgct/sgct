/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
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

	mWorldTransform[0] = glm::mat4(1.0f);
	mWorldTransform[1] = glm::mat4(1.0f);
	mDeviceTransformMatrix = glm::mat4(1.0f);
	mSensorRotation[0] = glm::dquat(0.0, 0.0, 0.0, 0.0);
	mSensorRotation[1] = glm::dquat(0.0, 0.0, 0.0, 0.0);
	mSensorPos[0] = glm::dvec3(0.0, 0.0, 0.0);
	mSensorPos[1] = glm::dvec3(0.0, 0.0, 0.0);

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

void sgct::SGCTTrackingDevice::setSensorTransform(glm::dvec3 vec, glm::dquat rot)
{
	sgct::SGCTTracker * parent = sgct_core::ClusterManager::instance()->getTrackingManagerPtr()->getTrackerPtr(mParentIndex);

	if (parent == NULL)
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "SGCTTrackingDevice: Error, can't get handle to tracker for device '%s'!\n", mName.c_str());
		return;
	}
	
	glm::mat4 systemTransformMatrix = parent->getTransform();
	//convert from double to float
	glm::quat sensorRot(
		static_cast<float>(rot.w),
		static_cast<float>(rot.x),
		static_cast<float>(rot.y),
		static_cast<float>(rot.z));
	glm::vec3 sensorPos(vec);

	//create matrixes
	glm::mat4 sensorTransMat = glm::translate(glm::mat4(1.0f), sensorPos);
	glm::mat4 sensorRotMat(glm::mat4_cast(sensorRot));

	SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    
	//swap
	mSensorRotation[PREVIOUS] = mSensorRotation[CURRENT];
	mSensorRotation[CURRENT] = rot;

	mSensorPos[PREVIOUS] = mSensorPos[CURRENT];
	mSensorPos[CURRENT] = vec;

    mWorldTransform[PREVIOUS] = mWorldTransform[CURRENT];
	mWorldTransform[CURRENT] = systemTransformMatrix * sensorTransMat * sensorRotMat * mDeviceTransformMatrix;

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

void sgct::SGCTTrackingDevice::setButtonVal(const bool val, size_t index)
{
	if( index < mNumberOfButtons )
	{
		SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
		//swap
        mButtons[index + mNumberOfButtons] = mButtons[index];
        mButtons[index] = val;
		SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

		setButtonTimeStamp(index);
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
*/
void sgct::SGCTTrackingDevice::setOrientation(float xRot, float yRot, float zRot)
{
	//create rotation quaternion based on x, y, z rotations
	glm::quat rotQuat;
	rotQuat = glm::rotate(rotQuat, glm::radians(xRot), glm::vec3(1.0f, 0.0f, 0.0f));
	rotQuat = glm::rotate(rotQuat, glm::radians(yRot), glm::vec3(0.0f, 1.0f, 0.0f));
	rotQuat = glm::rotate(rotQuat, glm::radians(zRot), glm::vec3(0.0f, 0.0f, 1.0f));

	SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
	//create inverse rotation matrix
	mOrientation = rotQuat;

	calculateTransform();
	SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

/*!
Set the orientation quaternion used to generate the orientation matrix\n
*/
void sgct::SGCTTrackingDevice::setOrientation(float w, float x, float y, float z)
{
	glm::quat rotQuat(w, x, y, z);
	
	SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
	//create inverse rotation matrix
	mOrientation = rotQuat;

	calculateTransform();
	SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

/*!
Set the offset vector used to generate the offset matrix\n
*/
void sgct::SGCTTrackingDevice::setOffset(float x, float y, float z)
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
*/
void sgct::SGCTTrackingDevice::setTransform(glm::mat4 mat)
{
	/*fprintf(stderr, "----------------\nTransform %s:\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n",
		mName.c_str(),
		mat[0][0], mat[1][0], mat[2][0], mat[3][0],
		mat[0][1], mat[1][1], mat[2][1], mat[3][1],
		mat[0][2], mat[1][2], mat[2][2], mat[3][2],
		mat[0][3], mat[1][3], mat[2][3], mat[3][3]);*/

	SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
	mDeviceTransformMatrix = mat;
	SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

void sgct::SGCTTrackingDevice::calculateTransform()
{
	//create offset translation matrix
	glm::mat4 transMat = glm::translate( glm::mat4(1.0f), mOffset );
	//calculate transform
	mDeviceTransformMatrix = transMat * glm::mat4_cast(mOrientation);
}

/*!
\returns the id of this device/sensor
*/
int sgct::SGCTTrackingDevice::getSensorId()
{
	int tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get sensor id...\n");
#endif
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		tmpVal = mSensorId;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );
	return tmpVal;
}

/*!
\returns a digital value from array
*/
bool sgct::SGCTTrackingDevice::getButton(size_t index, DataLoc i)
{
	bool tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get button from array...\n");
#endif
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		tmpVal = index < mNumberOfButtons ? mButtons[index + mNumberOfButtons * i] : false;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );
	return tmpVal;
}

/*!
\returns an analog value from array
*/
double sgct::SGCTTrackingDevice::getAnalog(size_t index, DataLoc i)
{
	double tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get analog value...\n");
#endif
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		tmpVal = index < mNumberOfAxes ? mAxes[index + mNumberOfAxes * i] : 0.0;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );
	return tmpVal;
}

/*!
\returns the sensor's position in world coordinates
*/
glm::vec3 sgct::SGCTTrackingDevice::getPosition(DataLoc i)
{
	glm::vec3 tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get position...\n");
#endif
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		glm::mat4 & matRef = mWorldTransform[i];
		tmpVal[0] = matRef[3][0];
		tmpVal[1] = matRef[3][1];
		tmpVal[2] = matRef[3][2];
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );
	return tmpVal;
}

/*!
\returns the sensor's rotation as as euler angles in world coordinates
*/
glm::vec3 sgct::SGCTTrackingDevice::getEulerAngles(DataLoc i)
{
	glm::vec3 tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get euler angles");
#endif
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		tmpVal = glm::eulerAngles( glm::quat_cast(mWorldTransform[i]) );
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );
	return tmpVal;
}

/*!
\returns the sensor's rotation as a quaternion in world coordinates
*/
glm::quat sgct::SGCTTrackingDevice::getRotation(DataLoc i)
{
	glm::quat tmpQuat;
	SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
	tmpQuat = glm::quat_cast(mWorldTransform[i]);
	SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
	return tmpQuat;
}

/*!
\returns the sensor's transform matrix in world coordinates
*/
glm::mat4 sgct::SGCTTrackingDevice::getWorldTransform(DataLoc i)
{
	glm::mat4 tmpMat;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get transform matrix...\n");
#endif
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		tmpMat = mWorldTransform[i];
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );
	return tmpMat;
}

/*!
\returns the raw sensor rotation quaternion
*/
glm::dquat sgct::SGCTTrackingDevice::getSensorRotation(DataLoc i)
{
	glm::dquat tmpQuat;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
	fprintf(stderr, "Get sensor quaternion...\n");
#endif
	SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
	tmpQuat = mSensorRotation[i];
	SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
	return tmpQuat;
}

/*!
\returns the raw sensor position vector
*/
glm::dvec3 sgct::SGCTTrackingDevice::getSensorPosition(DataLoc i)
{
	glm::dvec3 tmpVec;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
	fprintf(stderr, "Get sensor position vector...\n");
#endif
	SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
	tmpVec = mSensorPos[i];
	SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
	return tmpVec;
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
