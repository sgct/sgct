/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _PROJECTION_PLANE_H
#define _PROJECTION_PLANE_H

#include <glm/glm.hpp>

#ifndef SGCT_DONT_USE_EXTERNAL
#include "external/tinyxml2.h"
#else
#include <tinyxml2.h>
#endif

namespace sgct_core
{

	/*!
	This class holds and manages the 3D projection plane
	*/
	class SGCTProjectionPlane
	{
	public:
		enum ProjectionPlaneCorner { LowerLeft = 0, UpperLeft, UpperRight };

		SGCTProjectionPlane();
		void configure(tinyxml2::XMLElement * element);
		void reset();
		void offset(glm::vec3 p);

		void setCoordinate(ProjectionPlaneCorner corner, glm::vec3 coordinate);
		void setCoordinate(std::size_t corner, glm::vec3 coordinate);
		const glm::vec3 * getCoordinatePtr(ProjectionPlaneCorner corner) const;
		glm::vec3 getCoordinate(ProjectionPlaneCorner corner) const;

	protected:
		glm::vec3 mProjectionPlaneCoords[3];
	};

}

#endif