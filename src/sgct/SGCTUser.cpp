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

#include "../include/sgct/SGCTUser.h"

sgct_core::SGCTUser::SGCTUser()
{
	for(unsigned int i=0; i<3; i++)
		mPos[i] = glm::vec3(0.0f);
	mEyeSeparation = 0.069f;
	mTransform = glm::dmat4(1.0);
}

void sgct_core::SGCTUser::setPos(float x, float y, float z)
{
	mPos[Frustum::Mono].x = x;
	mPos[Frustum::Mono].y = y;
	mPos[Frustum::Mono].z = z;
	updateEyeSeparation();
}

void sgct_core::SGCTUser::setPos(glm::vec3 pos)
{
	mPos[Frustum::Mono] = pos;
	updateEyeSeparation();
}

void sgct_core::SGCTUser::setPos(glm::dvec4 pos)
{
	mPos[Frustum::Mono] = glm::vec3(pos);
	updateEyeSeparation();
}

void sgct_core::SGCTUser::setPos(double * pos)
{
	mPos[Frustum::Mono].x = static_cast<float>(pos[0]);
	mPos[Frustum::Mono].y = static_cast<float>(pos[1]);
	mPos[Frustum::Mono].z = static_cast<float>(pos[2]);
	updateEyeSeparation();
}

void sgct_core::SGCTUser::setHeadTracker(const char * trackerName, const char * deviceName)
{
	mHeadTrackerDeviceName.assign(deviceName);
	mHeadTrackerName.assign(trackerName);
}

void sgct_core::SGCTUser::setTransform(const glm::dmat4 & transform)
{
	mTransform = glm::dmat4( transform );
	updateEyeTransform();
}

void sgct_core::SGCTUser::setTransform(const glm::mat4 & transform)
{
	mTransform = transform;
	updateEyeTransform();
}

void sgct_core::SGCTUser::setOrientation(float xRot, float yRot, float zRot)
{
	//create rotation quaternion based on x, y, z rotations
	glm::quat rotQuat;
	rotQuat = glm::rotate( rotQuat, xRot, glm::vec3(1.0f, 0.0f, 0.0f) );
	rotQuat = glm::rotate( rotQuat, yRot, glm::vec3(0.0f, 1.0f, 0.0f) );
	rotQuat = glm::rotate( rotQuat, zRot, glm::vec3(0.0f, 0.0f, 1.0f) );

	//create offset translation matrix
	glm::mat4 transMat = glm::translate( glm::mat4(1.0f), mPos[Frustum::Mono] );
	
	//calculate transform
	mTransform = transMat * glm::mat4_cast(rotQuat);

	updateEyeTransform();
}

void sgct_core::SGCTUser::setEyeSeparation(float eyeSeparation)
{
	mEyeSeparation = eyeSeparation;
	updateEyeSeparation();
}

void sgct_core::SGCTUser::updateEyeSeparation()
{
	glm::vec3 eyeOffsetVec( mEyeSeparation/2.0f, 0.0f, 0.0f );
	mPos[Frustum::StereoLeftEye] = mPos[Frustum::Mono] - eyeOffsetVec;
	mPos[Frustum::StereoRightEye] = mPos[Frustum::Mono] + eyeOffsetVec;
}

void sgct_core::SGCTUser::updateEyeTransform()
{
	glm::vec4 eyeOffsetVec( mEyeSeparation/2.0f, 0.0f, 0.0f, 0.0f );
	
	glm::vec4 pos[3];
	pos[Frustum::Mono] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	pos[Frustum::StereoLeftEye] = pos[Frustum::Mono] - eyeOffsetVec;
	pos[Frustum::StereoRightEye] = pos[Frustum::Mono] + eyeOffsetVec;

	mPos[Frustum::Mono] = glm::vec3( mTransform * pos[Frustum::Mono] );
	mPos[Frustum::StereoLeftEye] = glm::vec3( mTransform * pos[Frustum::StereoLeftEye] );
	mPos[Frustum::StereoRightEye] = glm::vec3( mTransform * pos[Frustum::StereoRightEye] );
}

const glm::vec3 & sgct_core::SGCTUser::getPos(Frustum::FrustumMode fm)
{
	return mPos[fm];
}