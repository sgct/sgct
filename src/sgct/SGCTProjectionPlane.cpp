/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include "../include/sgct/SGCTProjectionPlane.h"

sgct_core::SGCTProjectionPlane::SGCTProjectionPlane()
{
	reset();
}

void sgct_core::SGCTProjectionPlane::reset()
{
	mProjectionPlaneCoords[LowerLeft] = glm::vec3(-1.0f, -1.0f, -2.0f);
	mProjectionPlaneCoords[UpperLeft] = glm::vec3(-1.0f, 1.0f, -2.0f);
	mProjectionPlaneCoords[UpperRight] = glm::vec3(1.0f, 1.0f, -2.0f);
}

void sgct_core::SGCTProjectionPlane::setCoordinate(ProjectionPlaneCorner corner, glm::vec3 coordinate)
{
	mProjectionPlaneCoords[corner] = coordinate;
}

void sgct_core::SGCTProjectionPlane::setCoordinate(std::size_t corner, glm::vec3 coordinate)
{
	mProjectionPlaneCoords[corner] = coordinate;
}

/*!
\returns coordinate pointer for the selected projection plane corner
*/
const glm::vec3 * sgct_core::SGCTProjectionPlane::getCoordinatePtr(ProjectionPlaneCorner corner) const
{
	return &mProjectionPlaneCoords[corner];
}

/*!
\returns coordinate for selected the projection plane corner
*/
glm::vec3 sgct_core::SGCTProjectionPlane::getCoordinate(ProjectionPlaneCorner corner) const
{
	return mProjectionPlaneCoords[corner];
}