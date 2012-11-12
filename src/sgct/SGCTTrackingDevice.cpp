/*************************************************************************
Copyright (c) 2012 Miroslav Andel, Linköping University.
All rights reserved.

Original Authors:
Miroslav Andel, Alexander Fridlund

For any questions or information about the SGCT project please contact: miroslav.andel@liu.se

This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to
Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <GL/glfw.h>
#include "../include/sgct/ClusterManager.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/SGCTTrackingDevice.h"
#include "../include/sgct/Engine.h"

extern GLFWmutex gTrackingMutex;

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

void sgct::SGCTTrackingDevice::setEnabled(bool state)
{
	mEnabled = state;
}

void sgct::SGCTTrackingDevice::setSensorId(int id)
{
	mSensorId = id;
}

void sgct::SGCTTrackingDevice::setNumberOfButtons(size_t numOfButtons)
{
	if( mButtons == NULL )
	{
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
}

void sgct::SGCTTrackingDevice::setNumberOfAxes(size_t numOfAxes)
{
	if( mAxes == NULL )
	{
		//double buffered
		mAxes = new double[numOfAxes * 2];
		mNumberOfAxes = numOfAxes;
		for(size_t i=0; i<mNumberOfAxes; i++)
		{
			mAxes[i] = 0.0;
		}
	}
}

void sgct::SGCTTrackingDevice::setSensorTransform( glm::dmat4 mat )
{
	const glm::dmat4 & preTransform =
        sgct_core::ClusterManager::Instance()->getTrackingManagerPtr()->getTrackerPtr(mParentIndex)->getTransform();

    //swap
    mWorldTransform[PREVIOUS] = mWorldTransform[CURRENT];
    mWorldTransform[CURRENT] = (preTransform * mat) * mPostTransform;

    setTrackerTimeStamp();
}

void sgct::SGCTTrackingDevice::setButtonVal(const bool val, size_t index)
{
	if( index < mNumberOfButtons )
	{
		//swap
        mButtons[index + mNumberOfButtons] = mButtons[index];
        mButtons[index] = val;

		setButtonTimeStamp( index );
    }
}

void sgct::SGCTTrackingDevice::setAnalogVal(const double * array, size_t size)
{
	for( size_t i=0; i < size; i++ )
        if( i < mNumberOfAxes )
        {
            mAxes[i + mNumberOfAxes] = mAxes[i];
            mAxes[i] = array[i];
        }

    setAnalogTimeStamp();
}

void sgct::SGCTTrackingDevice::setOrientation(double xRot, double yRot, double zRot)
{
	//create rotation quaternion based on x, y, z rotations
	glm::dquat rotQuat;
	rotQuat = glm::rotate( rotQuat, xRot, glm::dvec3(1.0, 0.0, 0.0) );
	rotQuat = glm::rotate( rotQuat, yRot, glm::dvec3(0.0, 1.0, 0.0) );
	rotQuat = glm::rotate( rotQuat, zRot, glm::dvec3(0.0, 0.0, 1.0) );

	//create inverse rotation matrix
	mOrientation = glm::mat4_cast(rotQuat);

	calculateTransform();
}

void sgct::SGCTTrackingDevice::setOffset(double x, double y, double z)
{
	mOffset[0] = x;
	mOffset[1] = y;
	mOffset[2] = z;

	calculateTransform();
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
	Engine::lockMutex(gTrackingMutex);
		tmpVal = mSensorId;
	Engine::unlockMutex(gTrackingMutex);
	return tmpVal;
}

bool sgct::SGCTTrackingDevice::getButton(size_t index, DataLoc i)
{
	bool tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get button...\n");
#endif
	Engine::lockMutex(gTrackingMutex);
		tmpVal = index < mNumberOfButtons ? mButtons[index + mNumberOfButtons * i] : false;
	Engine::unlockMutex(gTrackingMutex);
	return tmpVal;
}

double sgct::SGCTTrackingDevice::getAnalog(size_t index, DataLoc i)
{
	double tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get analog array...\n");
#endif
	Engine::lockMutex(gTrackingMutex);
		tmpVal = index < mNumberOfAxes ? mAxes[index + mNumberOfAxes * i] : false;
	Engine::unlockMutex(gTrackingMutex);
	return tmpVal;
}

glm::dvec3 sgct::SGCTTrackingDevice::getPosition(DataLoc i)
{
	glm::dvec3 tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get position...\n");
#endif
	Engine::lockMutex(gTrackingMutex);
		glm::dmat4 & matRef = mWorldTransform[i];
		tmpVal[0] = matRef[3][0];
		tmpVal[1] = matRef[3][1];
		tmpVal[2] = matRef[3][2];
	Engine::unlockMutex(gTrackingMutex);
	return tmpVal;
}

glm::dvec3 sgct::SGCTTrackingDevice::getEulerAngles(DataLoc i)
{
	glm::dvec3 tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get euler angles");
#endif
	Engine::lockMutex(gTrackingMutex);
		tmpVal = glm::eulerAngles( glm::quat_cast(mWorldTransform[i]) );
	Engine::unlockMutex(gTrackingMutex);
	return tmpVal;
}

glm::dmat4 sgct::SGCTTrackingDevice::getTransformMat(DataLoc i)
{
	glm::dmat4 tmpMat;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get transform matrix...\n");
#endif
	Engine::lockMutex(gTrackingMutex);
		tmpMat = mWorldTransform[i];
	Engine::unlockMutex(gTrackingMutex);
	return tmpMat;
}

bool sgct::SGCTTrackingDevice::isEnabled()
{
    bool tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Is device enabled...\n");
#endif
	Engine::lockMutex(gTrackingMutex);
		tmpVal = mEnabled;
	Engine::unlockMutex(gTrackingMutex);

    return tmpVal;
}

void sgct::SGCTTrackingDevice::setTrackerTimeStamp()
{
	mTrackerTime[0] = sgct::Engine::getTime();
	//swap
	mTrackerTime[1] = mTrackerTime[0];
}

void sgct::SGCTTrackingDevice::setAnalogTimeStamp()
{
	mAnalogTime[0] = sgct::Engine::getTime();
	//swap
	mAnalogTime[1] = mAnalogTime[0];
}

void sgct::SGCTTrackingDevice::setButtonTimeStamp(size_t index)
{
	mButtonTime[index] = sgct::Engine::getTime();
	//swap
	mButtonTime[index + mNumberOfButtons] = mButtonTime[index];
}

double sgct::SGCTTrackingDevice::getTrackerTimeStamp(DataLoc i)
{
	double tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get device tracker time stamp...\n");
#endif
	Engine::lockMutex(gTrackingMutex);
		tmpVal = mTrackerTime[i];
	Engine::unlockMutex(gTrackingMutex);

	return tmpVal;
}

double sgct::SGCTTrackingDevice::getAnalogTimeStamp(DataLoc i)
{
	double tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get device analog time stamp...\n");
#endif
	Engine::lockMutex(gTrackingMutex);
		tmpVal = mAnalogTime[i];
	Engine::unlockMutex(gTrackingMutex);

	return tmpVal;
}

double sgct::SGCTTrackingDevice::getButtonTimeStamp(size_t index, DataLoc i)
{
	double tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get device button time stamp...\n");
#endif

	Engine::lockMutex(gTrackingMutex);
		tmpVal = mButtonTime[index + mNumberOfButtons * i];
	Engine::unlockMutex(gTrackingMutex);

	return tmpVal;
}

double sgct::SGCTTrackingDevice::getTrackerDeltaTime()
{
	double tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get device tracker delta time...\n");
#endif

	Engine::lockMutex(gTrackingMutex);
		tmpVal = mTrackerTime[0] - mTrackerTime[1];
	Engine::unlockMutex(gTrackingMutex);

	return tmpVal;
}

double sgct::SGCTTrackingDevice::getAnalogDeltaTime()
{
	double tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get device analog delta time...\n");
#endif

	Engine::lockMutex(gTrackingMutex);
		tmpVal = mAnalogTime[0] - mAnalogTime[1];
	Engine::unlockMutex(gTrackingMutex);

	return tmpVal;
}

double sgct::SGCTTrackingDevice::getButtonDeltaTime(size_t index)
{
	double tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get device button delta time...\n");
#endif

	Engine::lockMutex(gTrackingMutex);
		tmpVal = mButtonTime[index] - mButtonTime[index + mNumberOfButtons];
	Engine::unlockMutex(gTrackingMutex);

	return tmpVal;
}
