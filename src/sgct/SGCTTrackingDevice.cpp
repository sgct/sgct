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

extern GLFWmutex mTrackingMutex;

core_sgct::SGCTTrackingDevice::SGCTTrackingDevice(size_t index, const char * name)
{
	mEnabled = true;
	mIsPositionalDevice = false;
	mName.assign( name );
	mIndex = index;
	mNumberOfButtons = 0;
	mNumberOfAxes = 0;

	mWorldTransform[0] = glm::dmat4(1.0);
	mWorldTransform[1] = glm::dmat4(1.0);
	mPostTransform = glm::dmat4(1.0);

	mButtons = NULL;
	mAxes = NULL;
	mTrackerTime[0] = 0.0;
	mTrackerTime[1] = 0.0;
	mAnalogTime[0] = 0.0;
	mAnalogTime[1] = 0.0;
	mSensor = -1;
}

core_sgct::SGCTTrackingDevice::~SGCTTrackingDevice()
{
	mEnabled = false;

	if( mButtons != NULL )
	{
		delete [] mButtons;
		mButtons = NULL;
	}

	if( mAxes != NULL )
	{
		delete [] mAxes;
		mAxes = NULL;
	}
}

void core_sgct::SGCTTrackingDevice::setEnabled(bool state)
{
	mEnabled = state;
}

void core_sgct::SGCTTrackingDevice::setSensor(int sensor)
{
	mSensor = sensor;
}

void core_sgct::SGCTTrackingDevice::setPositionalDevicePresent(bool state)
{
	mIsPositionalDevice = state;
}

void core_sgct::SGCTTrackingDevice::setNumberOfButtons(size_t numOfButtons)
{
	if( mButtons == NULL )
	{
		//double buffered
		mButtons = new bool[numOfButtons * 2];
		mNumberOfButtons = numOfButtons;
		for(size_t i=0; i<mNumberOfButtons; i++)
		{
			mButtons[i] = false;
		}
	}
}

void core_sgct::SGCTTrackingDevice::setNumberOfAxes(size_t numOfAxes)
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

void core_sgct::SGCTTrackingDevice::setSensorTransform( glm::dmat4 mat )
{
	const glm::dmat4 & preTransform =
        ClusterManager::Instance()->getTrackingManagerPtr()->getTransform();

    //swap
    mWorldTransform[PREVIOUS] = mWorldTransform[CURRENT];
    mWorldTransform[CURRENT] = (preTransform * mat) * mPostTransform;

    setTrackerTime();
}

void core_sgct::SGCTTrackingDevice::setButtonVal(const bool val, size_t index)
{
	if( index < mNumberOfButtons )
	{
		sgct::Engine::lockMutex(mTrackingMutex);
			//swap
			mButtons[index + mNumberOfButtons] = mButtons[index];
			mButtons[index] = val;
		sgct::Engine::unlockMutex(mTrackingMutex);
	}
}

void core_sgct::SGCTTrackingDevice::setAnalogVal(const double * array, size_t size)
{
	for( size_t i=0; i < size; i++ )
        if( i < mNumberOfAxes )
        {
            mAxes[i + mNumberOfAxes] = mAxes[i];
            mAxes[i] = array[i];
        }

    setAnalogTime();
}

void core_sgct::SGCTTrackingDevice::setOrientation(double xRot, double yRot, double zRot)
{
	//create rotation quaternion based on x, y, z rotations
	glm::dquat rotQuat;
	rotQuat = glm::rotate( rotQuat, xRot, glm::dvec3(1.0, 0.0, 0.0) );
	rotQuat = glm::rotate( rotQuat, yRot, glm::dvec3(0.0, 1.0, 0.0) );
	rotQuat = glm::rotate( rotQuat, zRot, glm::dvec3(0.0, 0.0, 1.0) );

	//create inverse rotation matrix
	mOrientation = glm::inverse( glm::mat4_cast(rotQuat) );

	calculateTransform();
}

void core_sgct::SGCTTrackingDevice::setOffset(double x, double y, double z)
{
	mOffset[0] = x;
	mOffset[1] = y;
	mOffset[2] = z;

	calculateTransform();
}

void core_sgct::SGCTTrackingDevice::calculateTransform()
{
	//create offset translation matrix
	glm::dmat4 transMat = glm::translate( glm::dmat4(1.0), mOffset );
	//calculate transform
	mPostTransform = transMat * mOrientation;
}

int core_sgct::SGCTTrackingDevice::getSensor()
{
	int tmpVal;
	sgct::Engine::lockMutex(mTrackingMutex);
		tmpVal = mSensor;
	sgct::Engine::unlockMutex(mTrackingMutex);
	return tmpVal;
}

bool core_sgct::SGCTTrackingDevice::getButton(size_t index, DataLoc i)
{
	bool tmpVal;
	sgct::Engine::lockMutex(mTrackingMutex);
		tmpVal = index < mNumberOfButtons ? mButtons[index + mNumberOfButtons * i] : false;
	sgct::Engine::unlockMutex(mTrackingMutex);
	return tmpVal;
}

double core_sgct::SGCTTrackingDevice::getAnalog(size_t index, DataLoc i)
{
	double tmpVal;
	sgct::Engine::lockMutex(mTrackingMutex);
		tmpVal = index < mNumberOfAxes ? mAxes[index + mNumberOfAxes * i] : false;
	sgct::Engine::unlockMutex(mTrackingMutex);
	return tmpVal;
}

glm::dvec3 core_sgct::SGCTTrackingDevice::getPosition(DataLoc i)
{
	glm::dvec3 tmpVal;
	sgct::Engine::lockMutex(mTrackingMutex);
		glm::dmat4 & matRef = mWorldTransform[i];
		tmpVal[0] = matRef[3][0];
		tmpVal[1] = matRef[3][1];
		tmpVal[2] = matRef[3][2];
	sgct::Engine::unlockMutex(mTrackingMutex);
	return tmpVal;
}

glm::dvec3 core_sgct::SGCTTrackingDevice::getEulerAngles(DataLoc i)
{
	glm::dvec3 tmpVal;
	sgct::Engine::lockMutex(mTrackingMutex);
		tmpVal = glm::eulerAngles( glm::quat_cast(mWorldTransform[i]) );
	sgct::Engine::unlockMutex(mTrackingMutex);
	return tmpVal;
}

glm::dmat4 core_sgct::SGCTTrackingDevice::getTransformMat(DataLoc i)
{
	glm::dmat4 tmpMat;
	sgct::Engine::lockMutex(mTrackingMutex);
		tmpMat = mWorldTransform[i];
	sgct::Engine::unlockMutex(mTrackingMutex);
	return tmpMat;
}

bool core_sgct::SGCTTrackingDevice::isEnabled()
{
    bool tmpVal;
	sgct::Engine::lockMutex(mTrackingMutex);
		tmpVal = mEnabled;
	sgct::Engine::unlockMutex(mTrackingMutex);

    return tmpVal;
}

void core_sgct::SGCTTrackingDevice::setTrackerTime()
{
	double current_time = sgct::Engine::getTime();
    mTrackerTime[0] = current_time - mTrackerTime[1];
	mTrackerTime[1] = current_time;
}

double core_sgct::SGCTTrackingDevice::getTrackerTime()
{
	double tmpVal;
	sgct::Engine::lockMutex(mTrackingMutex);
		tmpVal = mTrackerTime[0];
	sgct::Engine::unlockMutex(mTrackingMutex);

	return tmpVal;
}

void core_sgct::SGCTTrackingDevice::setAnalogTime()
{
	double current_time = sgct::Engine::getTime();
	mAnalogTime[0] = current_time - mAnalogTime[1];
	mAnalogTime[1] = current_time;
}

double core_sgct::SGCTTrackingDevice::getAnalogTime()
{
	double tmpVal;
	sgct::Engine::lockMutex(mTrackingMutex);
		tmpVal = mAnalogTime[0];
	sgct::Engine::unlockMutex(mTrackingMutex);

	return tmpVal;
}
