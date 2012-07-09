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

#ifndef _USER_H_
#define _USER_H_

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Frustum.h"

namespace core_sgct
{

/*!
Helper class for setting user variables
*/
class User
{
public:
	User()
	{
		for(unsigned int i=0; i<3; i++)
			mPos[i] = glm::vec3(0.0f);
		mEyeSeparation = 0.069f;
		mOrientation = glm::mat3(1.0f);
	}

	void setPos(float x, float y, float z)
	{
		mPos[Frustum::Mono].x = x;
		mPos[Frustum::Mono].y = y;
		mPos[Frustum::Mono].z = z;
		updateEyeSeparation();
	}

	void setPos(glm::vec3 pos)
	{
		mPos[Frustum::Mono] = pos;
		updateEyeSeparation();
	}

	void setPos(glm::dvec4 pos)
	{
		mPos[Frustum::Mono] = glm::vec3(pos);
		updateEyeSeparation();
	}

	void setPos(double * pos)
	{
		mPos[Frustum::Mono].x = static_cast<float>(pos[0]);
		mPos[Frustum::Mono].y = static_cast<float>(pos[1]);
		mPos[Frustum::Mono].z = static_cast<float>(pos[2]);
		updateEyeSeparation();
	}

	void setOrientation(const glm::dmat3 & rotMat)
	{
		mOrientation = rotMat;
	}

	void setEyeSeparation(float eyeSeparation)
	{
		mEyeSeparation = eyeSeparation;
		updateEyeSeparation();
	}

	glm::vec3 getPos() { return mPos[Frustum::Mono]; }
	glm::mat3 getOrientation() { return mOrientation; }
	glm::vec3 getPos(Frustum::FrustumMode fm) { return mPos[fm]; }
	glm::vec3 * getPosPtr() { return &mPos[Frustum::Mono]; }
	glm::vec3 * getPosPtr(Frustum::FrustumMode fm) { return &mPos[fm]; }

	inline float getEyeSeparation() { return mEyeSeparation; }
	inline float getXPos() { return mPos[Frustum::Mono].x; }
	inline float getYPos() { return mPos[Frustum::Mono].y; }
	inline float getZPos() { return mPos[Frustum::Mono].z; }

private:
	void updateEyeSeparation()
	{
		glm::vec3 eyeOffsetVec = glm::vec3( mEyeSeparation/2.0f, 0.0f, 0.0f );
		glm::vec3 rotatedEyeOffsetVec = mOrientation * eyeOffsetVec;

		mPos[Frustum::StereoLeftEye] = mPos[Frustum::Mono] - rotatedEyeOffsetVec;
		mPos[Frustum::StereoRightEye] = mPos[Frustum::Mono] + rotatedEyeOffsetVec;
	}

private:
	glm::vec3 mPos[3];
	glm::mat3 mOrientation;
	float mEyeSeparation;
};

} // sgct

#endif