/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/SGCTUser.h"

sgct_core::SGCTUser::SGCTUser()
{
	for(unsigned int i=0; i<3; i++)
		mPos[i] = glm::vec3(0.0f);
	mEyeSeparation = 0.069f;
	mHalfEyeSeparation = mEyeSeparation / 2.0f;
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
	mHalfEyeSeparation = mEyeSeparation / 2.0f;
	updateEyeSeparation();
}

void sgct_core::SGCTUser::updateEyeSeparation()
{
	glm::vec3 eyeOffsetVec( mHalfEyeSeparation, 0.0f, 0.0f );
	mPos[Frustum::StereoLeftEye] = mPos[Frustum::Mono] - eyeOffsetVec;
	mPos[Frustum::StereoRightEye] = mPos[Frustum::Mono] + eyeOffsetVec;
}

void sgct_core::SGCTUser::updateEyeTransform()
{
	glm::vec4 eyeOffsetVec( mHalfEyeSeparation, 0.0f, 0.0f, 0.0f );
	
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
