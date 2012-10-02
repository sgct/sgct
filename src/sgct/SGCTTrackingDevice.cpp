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

core_sgct::SGCTTrackingDevice::SGCTTrackingDevice(size_t index, const char * name)
{
	mEnabled = true;
	mIsPositionalDevice = false;
	mName.assign( name );
	mIndex = index;
	mNumberOfButtons = 0;
	mNumberOfAxes = 0;
	mTrackedPos = glm::dvec4(0.0);
	mRotationMat = glm::dmat4(1.0);

	mButtons = NULL;
	mAxes = NULL;
	mTrackerTime = 0.0;
	mLastTime = 0.0;

	mTrackingMutex = NULL;
	mTrackingMutex = sgct::Engine::createMutex();
	if( mTrackingMutex == NULL )
	{
		sgct::MessageHandler::Instance()->print("Tracking: Failed to create mutex!\n");
		mEnabled = false;
	}
}

core_sgct::SGCTTrackingDevice::~SGCTTrackingDevice()
{
	mEnabled = false;

	//destroy mutex
	if( mTrackingMutex != NULL )
	{
		sgct::Engine::destroyMutex(mTrackingMutex);
		mTrackingMutex = NULL;
	}

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

void core_sgct::SGCTTrackingDevice::setPositionalDevicePresent(bool state)
{
	mIsPositionalDevice = state;
}

void core_sgct::SGCTTrackingDevice::setNumberOfButtons(size_t numOfButtons)
{
	if( mButtons == NULL )
	{
		mButtons = new bool[numOfButtons];
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
		mAxes = new double[numOfAxes];
		mNumberOfAxes = numOfAxes;
		for(size_t i=0; i<mNumberOfAxes; i++)
		{
			mAxes[i] = 0.0;
		}
	}
}

void core_sgct::SGCTTrackingDevice::setPosition(const glm::dvec4 &pos)
{
	sgct::Engine::lockMutex(mTrackingMutex);
		mTrackedPos = pos;
	sgct::Engine::unlockMutex(mTrackingMutex);
}

void core_sgct::SGCTTrackingDevice::setRotation(const double &w, const double &x, const double &y, const double &z)
{
	sgct::Engine::lockMutex(mTrackingMutex);
		mRotation = glm::dquat(w, x, y, z);
		mRotationMat = ClusterManager::Instance()->getTrackingManagerPtr()->getOrientation() * glm::mat4_cast(mRotation);
	sgct::Engine::unlockMutex(mTrackingMutex);
}

void core_sgct::SGCTTrackingDevice::setButtonVal(const bool val, size_t index)
{
	if( index < mNumberOfButtons )
	{
		sgct::Engine::lockMutex(mTrackingMutex);
			mButtons[index] = val;
		sgct::Engine::unlockMutex(mTrackingMutex);
	}
}

void core_sgct::SGCTTrackingDevice::setAnalogVal(const double &val, size_t index)
{
	if( index < mNumberOfAxes )
	{
		sgct::Engine::lockMutex(mTrackingMutex);
			mAxes[index] = val;
		sgct::Engine::unlockMutex(mTrackingMutex);
	}
}

bool core_sgct::SGCTTrackingDevice::getButton(size_t index)
{
	bool tmpVal;
	sgct::Engine::lockMutex(mTrackingMutex);
		tmpVal = index < mNumberOfButtons ? mButtons[index] : false;
	sgct::Engine::unlockMutex(mTrackingMutex);
	return tmpVal;
}

double core_sgct::SGCTTrackingDevice::getAnalog(size_t index)
{
	double tmpVal;
	sgct::Engine::lockMutex(mTrackingMutex);
		tmpVal = index < mNumberOfAxes ? mAxes[index] : false;
	sgct::Engine::unlockMutex(mTrackingMutex);
	return tmpVal;
}

glm::dvec4 core_sgct::SGCTTrackingDevice::getPosition()
{
	glm::dvec4 tmpVal;
	sgct::Engine::lockMutex(mTrackingMutex);
		tmpVal = mTrackedPos;
	sgct::Engine::unlockMutex(mTrackingMutex);
	return tmpVal;
}

glm::dmat4 core_sgct::SGCTTrackingDevice::getRotationMat()
{
	glm::dmat4 tmpVal;
	sgct::Engine::lockMutex(mTrackingMutex);
		tmpVal = mRotationMat;
	sgct::Engine::unlockMutex(mTrackingMutex);
	return tmpVal;
}

glm::dvec3 core_sgct::SGCTTrackingDevice::getEulerAngles()
{
	glm::dvec3 tmpVal;
	sgct::Engine::lockMutex(mTrackingMutex);
		tmpVal = glm::eulerAngles( mRotation );
	sgct::Engine::unlockMutex(mTrackingMutex);
	return tmpVal;
}

glm::dmat4 core_sgct::SGCTTrackingDevice::getTransformMat()
{
	glm::dmat4 transMat;
	glm::dmat4 tmpMat;
	sgct::Engine::lockMutex(mTrackingMutex);
		transMat = glm::translate(glm::dmat4(1.0), glm::dvec3( mTrackedPos ) );
		tmpMat = mRotationMat * transMat;
	sgct::Engine::unlockMutex(mTrackingMutex);
	return tmpMat;
}

void core_sgct::SGCTTrackingDevice::setTrackerTime()
{
	sgct::Engine::lockMutex(mTrackingMutex);
		mTrackerTime = sgct::Engine::getTime() - mLastTime;
		mLastTime = mTrackerTime;
	sgct::Engine::unlockMutex(mTrackingMutex);
}

double core_sgct::SGCTTrackingDevice::getTrackerTime()
{
	double tmpVal;
	sgct::Engine::lockMutex(mTrackingMutex);
		tmpVal = mTrackerTime;
	sgct::Engine::unlockMutex(mTrackingMutex);

	return tmpVal;
}
