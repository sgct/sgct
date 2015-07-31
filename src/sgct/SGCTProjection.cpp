/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include "../include/sgct/SGCTProjection.h"
#include <glm/gtc/matrix_transform.hpp>

sgct_core::SGCTProjection::SGCTProjection()
{
	mViewMatrix = glm::mat4(1.0f);
	mProjectionMatrix = glm::mat4(1.0f);
	mViewProjectionMatrix = glm::mat4(1.0f);
}

void sgct_core::SGCTProjection::calculateProjection(glm::vec3 base, SGCTProjectionPlane * projectionPlanePtr, float near_clipping_plane, float far_clipping_plane, glm::vec3 viewOffset)
{
	glm::vec3 lowerLeftCorner = projectionPlanePtr->getCoordinate(SGCTProjectionPlane::LowerLeft);
	glm::vec3 upperLeftCorner = projectionPlanePtr->getCoordinate(SGCTProjectionPlane::UpperLeft);
	glm::vec3 upperRightCorner = projectionPlanePtr->getCoordinate(SGCTProjectionPlane::UpperRight);
	
	//calculate viewplane's internal coordinate system bases
	glm::vec3 plane_x = upperRightCorner - upperLeftCorner;
	glm::vec3 plane_y = upperLeftCorner - lowerLeftCorner;
	glm::vec3 plane_z = glm::cross(plane_x, plane_y);

	//normalize
	plane_x = glm::normalize(plane_x);
	plane_y = glm::normalize(plane_y);
	plane_z = glm::normalize(plane_z);

	glm::vec3 world_x(1.0f, 0.0f, 0.0f);
	glm::vec3 world_y(0.0f, 1.0f, 0.0f);
	glm::vec3 world_z(0.0f, 0.0f, 1.0f);

	//calculate plane rotation using
	//Direction Cosine Matrix (DCM)
	glm::mat3 DCM(1.0f); //init as identity matrix
	DCM[0][0] = glm::dot(plane_x, world_x);
	DCM[0][1] = glm::dot(plane_x, world_y);
	DCM[0][2] = glm::dot(plane_x, world_z);

	DCM[1][0] = glm::dot(plane_y, world_x);
	DCM[1][1] = glm::dot(plane_y, world_y);
	DCM[1][2] = glm::dot(plane_y, world_z);

	DCM[2][0] = glm::dot(plane_z, world_x);
	DCM[2][1] = glm::dot(plane_z, world_y);
	DCM[2][2] = glm::dot(plane_z, world_z);

	//invert & transform
	glm::mat3 DCM_inv = glm::inverse(DCM);
	glm::vec3 transformedViewPlaneCoords[3];
	transformedViewPlaneCoords[SGCTProjectionPlane::LowerLeft] = DCM_inv * lowerLeftCorner;
	transformedViewPlaneCoords[SGCTProjectionPlane::UpperLeft] = DCM_inv * upperLeftCorner;
	transformedViewPlaneCoords[SGCTProjectionPlane::UpperRight] = DCM_inv * upperRightCorner;
	glm::vec3 transformedEyePos = DCM_inv * base;

	//nearFactor = near clipping plane / focus plane dist
	float nearFactor = near_clipping_plane / (transformedViewPlaneCoords[SGCTProjectionPlane::LowerLeft].z - transformedEyePos.z);
	if (nearFactor < 0)
		nearFactor = -nearFactor;

	mFrustum.set(
		(transformedViewPlaneCoords[SGCTProjectionPlane::LowerLeft].x - transformedEyePos.x)*nearFactor,
		(transformedViewPlaneCoords[SGCTProjectionPlane::UpperRight].x - transformedEyePos.x)*nearFactor,
		(transformedViewPlaneCoords[SGCTProjectionPlane::LowerLeft].y - transformedEyePos.y)*nearFactor,
		(transformedViewPlaneCoords[SGCTProjectionPlane::UpperRight].y - transformedEyePos.y)*nearFactor,
		near_clipping_plane,
		far_clipping_plane);

	mViewMatrix = glm::mat4(DCM_inv) * glm::translate(glm::mat4(1.0f), -(base+viewOffset));

	//calc frustum matrix
	mProjectionMatrix = glm::frustum(
		mFrustum.getLeft(),
		mFrustum.getRight(),
		mFrustum.getBottom(),
		mFrustum.getTop(),
		mFrustum.getNear(),
		mFrustum.getFar());

	mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;
}