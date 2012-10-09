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
	Engine::lockMutex( gTrackingMutex );
		//create rotation quaternion based on x, y, z rotations
		glm::dquat rotQuat;
		rotQuat = glm::rotate( rotQuat, xRot, glm::dvec3(1.0, 0.0, 0.0) );
		rotQuat = glm::rotate( rotQuat, yRot, glm::dvec3(0.0, 1.0, 0.0) );
		rotQuat = glm::rotate( rotQuat, zRot, glm::dvec3(0.0, 0.0, 1.0) );

		//create inverse rotation matrix
		mOrientation = glm::inverse( glm::mat4_cast(rotQuat) );

		calculateTransform();
	Engine::unlockMutex( gTrackingMutex );
}

void sgct::SGCTTracker::setOffset(double x, double y, double z)
{
	Engine::lockMutex( gTrackingMutex );
		mOffset[0] = x;
		mOffset[1] = y;
		mOffset[2] = z;

		calculateTransform();
	Engine::unlockMutex( gTrackingMutex );
}

void sgct::SGCTTracker::setScale(double scaleVal)
{
	Engine::lockMutex( gTrackingMutex );
		if( scaleVal > 0.0 )
			mScale = scaleVal;
	Engine::unlockMutex( gTrackingMutex );
}

void sgct::SGCTTracker::calculateTransform()
{
	//create offset translation matrix
	glm::dmat4 transMat = glm::translate( glm::dmat4(1.0), mOffset );
	//calculate transform
	mXform = transMat * mOrientation;
}