/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/SGCTUser.h"
#include <glm/gtx/euler_angles.hpp>

/*!
	Default contructor
*/
sgct_core::SGCTUser::SGCTUser(std::string name)
{
	mName = name;
	
	for(unsigned int i=0; i<3; i++)
		mPos[i] = glm::vec3(0.0f);
	mEyeSeparation = 0.06f;
	mHalfEyeSeparation = mEyeSeparation / 2.0f;
	mTransform = glm::dmat4(1.0);
}

/*!
	Sets user's head position
*/
void sgct_core::SGCTUser::setPos(float x, float y, float z)
{
	mPos[Frustum::MonoEye].x = x;
	mPos[Frustum::MonoEye].y = y;
	mPos[Frustum::MonoEye].z = z;
	updateEyeSeparation();
}

/*!
	Sets user's head position
*/
void sgct_core::SGCTUser::setPos(glm::vec3 pos)
{
	mPos[Frustum::MonoEye] = pos;
	updateEyeSeparation();
}

/*!
	Sets user's head position
*/
void sgct_core::SGCTUser::setPos(glm::dvec4 pos)
{
	mPos[Frustum::MonoEye] = glm::vec3(pos);
	updateEyeSeparation();
}

/*!
	Sets user's head position
	@param pos double array with 3 slots for x, y & z
*/
void sgct_core::SGCTUser::setPos(float * pos)
{
	mPos[Frustum::MonoEye].x = pos[0];
	mPos[Frustum::MonoEye].y = pos[1];
	mPos[Frustum::MonoEye].z = pos[2];
	updateEyeSeparation();
}

/*!
	Set if the user's head position & orientation should be managed by a VRPN tracking device.
	This is normally done using the XML configuration file.

	@param trackerName the pointer to the tracker
	@param deviceName the name of the device which is mapped to the tracker
*/
void sgct_core::SGCTUser::setHeadTracker(const char * trackerName, const char * deviceName)
{
	mHeadTrackerDeviceName.assign(deviceName);
	mHeadTrackerName.assign(trackerName);
}

/*!
	Set the user's head transform matrix.
	@param transform the transform matrix
*/
void sgct_core::SGCTUser::setTransform(const glm::dmat4 & transform)
{
	mTransform = glm::mat4( transform );
	updateEyeTransform();
}

/*!
	Set the user's head transform matrix.
	@param transform the transform matrix
*/
void sgct_core::SGCTUser::setTransform(const glm::mat4 & transform)
{
	mTransform = transform;
	updateEyeTransform();
}

/*!
	Set the user's head orientation using euler angles.
	Note that rotations are dependent of each other, total rotation = xRot * yRot * zRot
	@param xRot the rotation around the x-azis
	@param yRot the rotation around the y-azis
	@param zRot the rotation around the z-azis
*/
void sgct_core::SGCTUser::setOrientation(float xRot, float yRot, float zRot)
{
	//create rotation quaternion based on x, y, z rotations
	/*glm::quat rotQuat;
	rotQuat = glm::rotate( rotQuat, glm::radians(xRot), glm::vec3(1.0f, 0.0f, 0.0f) );
	rotQuat = glm::rotate( rotQuat, glm::radians(yRot), glm::vec3(0.0f, 1.0f, 0.0f) );
	rotQuat = glm::rotate( rotQuat, glm::radians(zRot), glm::vec3(0.0f, 0.0f, 1.0f) );*/

	//create offset translation matrix
	glm::mat4 transMat = glm::translate(glm::mat4(1.0f), mPos[Frustum::MonoEye]);
	
	//calculate transform
	//mTransform = transMat * glm::mat4_cast(rotQuat);

	mTransform = transMat * glm::eulerAngleX( xRot ) * glm::eulerAngleY( yRot ) * glm::eulerAngleZ( zRot );

	updateEyeTransform();
}

/*!
Set the user's head orientation using a quaternion
*/
void sgct_core::SGCTUser::setOrientation(glm::quat q)
{
	//create offset translation matrix
	glm::mat4 transMat = glm::translate(glm::mat4(1.0f), mPos[Frustum::MonoEye]);

	mTransform = transMat * glm::mat4_cast( q );

	updateEyeTransform();
}

/*!
	Changes the interocular distance and recalculates the user's eye positions.
*/
void sgct_core::SGCTUser::setEyeSeparation(float eyeSeparation)
{
	mEyeSeparation = eyeSeparation;
	mHalfEyeSeparation = mEyeSeparation / 2.0f;
	updateEyeSeparation();
}

/*!
	Recalculates the user's eye positions based on head position and eye separation (interocular distance).
*/
void sgct_core::SGCTUser::updateEyeSeparation()
{
	glm::vec3 eyeOffsetVec( mHalfEyeSeparation, 0.0f, 0.0f );
	mPos[Frustum::StereoLeftEye] = mPos[Frustum::MonoEye] - eyeOffsetVec;
	mPos[Frustum::StereoRightEye] = mPos[Frustum::MonoEye] + eyeOffsetVec;
}

/*!
	Recalculates the user's eye orientations based on head position and eye separation (interocular distance).
*/
void sgct_core::SGCTUser::updateEyeTransform()
{
	glm::vec4 eyeOffsetVec( mHalfEyeSeparation, 0.0f, 0.0f, 0.0f );
	
	glm::vec4 pos[3];
	pos[Frustum::MonoEye] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	pos[Frustum::StereoLeftEye] = pos[Frustum::MonoEye] - eyeOffsetVec;
	pos[Frustum::StereoRightEye] = pos[Frustum::MonoEye] + eyeOffsetVec;

	mPos[Frustum::MonoEye] = glm::vec3(mTransform * pos[Frustum::MonoEye]);
	mPos[Frustum::StereoLeftEye] = glm::vec3( mTransform * pos[Frustum::StereoLeftEye] );
	mPos[Frustum::StereoRightEye] = glm::vec3( mTransform * pos[Frustum::StereoRightEye] );
}

/*!
	Get the users position (eye position)
	@param fm which eye/projection
	@returns position vector
*/
const glm::vec3 & sgct_core::SGCTUser::getPos(Frustum::FrustumMode fm)
{
	return mPos[fm];
}


/*!
Get the users name
@returns users name
*/
std::string sgct_core::SGCTUser::getName()
{
	return mName;
}
