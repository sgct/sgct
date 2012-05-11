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
#include "../include/sgct/SGCTTracking.h"
#include "../include/sgct/ClusterManager.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/vrpn/vrpn_Tracker.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

vrpn_Tracker_Remote * mTracker = NULL;
void VRPN_CALLBACK update_position_cb(void *userdata, const vrpn_TRACKERCB t);

core_sgct::SGCTTracking::SGCTTracking()
{
	mHeadSensorIndex = -1;
	mXform = glm::dmat4(1.0);
	mOffset = glm::dvec3(0.0);
	mEnabled = false;
}

core_sgct::SGCTTracking::~SGCTTracking()
{
	mEnabled = false;
	delete mTracker;
	mTracker = NULL;
}

void core_sgct::SGCTTracking::connect(const char * name)
{
	mTracker = new vrpn_Tracker_Remote(name);
	mTracker->register_change_handler(this, update_position_cb);
	mEnabled = true;
	sgct::MessageHandler::Instance()->print("Tracking: Connecting to %s...\n", name);
}

void core_sgct::SGCTTracking::update()
{
	if(mEnabled && mTracker != NULL)
	{
		mTracker->mainloop();
	}
}

void core_sgct::SGCTTracking::setHeadSensorIndex(int index)
{
	mHeadSensorIndex = index;
}

void core_sgct::SGCTTracking::setEnabled(bool state)
{
	mEnabled = state;
}

void core_sgct::SGCTTracking::setOrientation(double xRot, double yRot, double zRot)
{
	mXrot = xRot;
	mYrot = yRot;
	mZrot = zRot;

	calculateXform();
}

void core_sgct::SGCTTracking::setOffset(double x, double y, double z)
{
	mOffset[0] = x;
	mOffset[1] = y;
	mOffset[2] = z;

	calculateXform();
}

void core_sgct::SGCTTracking::calculateXform()
{
	//create rotation quaternion based on x, y, z rotations
	glm::dquat rotQuat;
	rotQuat = glm::rotate( rotQuat, mXrot, glm::dvec3(1.0, 0.0, 0.0) );
	rotQuat = glm::rotate( rotQuat, mYrot, glm::dvec3(0.0, 1.0, 0.0) );
	rotQuat = glm::rotate( rotQuat, mZrot, glm::dvec3(0.0, 0.0, 1.0) );
	
	//create inverse rotation matrix
	mOrientation = glm::inverse( glm::mat4_cast(rotQuat) );

	//create offset translation matrix
	glm::dmat4 transMat = glm::translate( glm::dmat4(1.0), mOffset );
	//calculate transform
	mXform = transMat * mOrientation;
}

void VRPN_CALLBACK update_position_cb(void *userdata, const vrpn_TRACKERCB info)
{
	if(info.sensor != -1)
	{
		core_sgct::SGCTTracking * trackerPtr = reinterpret_cast<core_sgct::SGCTTracking *>(userdata);
		
		if(info.sensor == trackerPtr->getHeadSensorIndex() && trackerPtr->getHeadSensorIndex() != -1)
		{
			glm::dvec4 tracked_pos = glm::dvec4( info.pos[0], info.pos[1], info.pos[2], 1.0 );
			glm::dquat tracked_rot = glm::dquat( info.quat[0], info.quat[1], info.quat[2], info.quat[3] );

			core_sgct::ClusterManager::Instance()->getUserPtr()->setOrientation( glm::dmat3(trackerPtr->getOrientation()) * glm::mat3_cast(tracked_rot) );
			core_sgct::ClusterManager::Instance()->getUserPtr()->setPos( trackerPtr->getXform() * tracked_pos );
		}
	}
}
