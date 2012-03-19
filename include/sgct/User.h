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
	}

	void setPos(float x, float y, float z)
	{
		mPos[Frustum::Mono].x = x;
		mPos[Frustum::Mono].y = y;
		mPos[Frustum::Mono].z = z;
		updateEyeSeparation();
	}

	void setPos(double * pos)
	{
		mPos[Frustum::Mono].x = static_cast<float>(pos[0]);
		mPos[Frustum::Mono].y = static_cast<float>(pos[1]);
		mPos[Frustum::Mono].z = static_cast<float>(pos[2]);
		updateEyeSeparation();
	}

	void setEyeSeparation(float eyeSeparation)
	{
		mEyeSeparation = eyeSeparation;
		updateEyeSeparation();
	}

	glm::vec3 getPos() { return mPos[Frustum::Mono]; }
	glm::vec3 getPos(Frustum::FrustumMode fm) { return mPos[fm]; }
	glm::vec3 * getPosPtr() { return &mPos[Frustum::Mono]; }
	glm::vec3 * getPosPtr(Frustum::FrustumMode fm) { return &mPos[fm]; }

	float getEyeSeparation() { return mEyeSeparation; }

private:
	void updateEyeSeparation()
	{
		mPos[Frustum::StereoLeftEye].x = mPos[Frustum::Mono].x - mEyeSeparation/2.0f;
		mPos[Frustum::StereoLeftEye].y = mPos[Frustum::Mono].y;
		mPos[Frustum::StereoLeftEye].z = mPos[Frustum::Mono].z;

		mPos[Frustum::StereoRightEye].x = mPos[Frustum::Mono].x + mEyeSeparation/2.0f;
		mPos[Frustum::StereoRightEye].y = mPos[Frustum::Mono].y;
		mPos[Frustum::StereoRightEye].z = mPos[Frustum::Mono].z;
	}

private:
	glm::vec3 mPos[3];
	float mEyeSeparation;
};

} // sgct

#endif