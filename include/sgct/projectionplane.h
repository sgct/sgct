/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__PROJECTION_PLANE__H__
#define __SGCT__PROJECTION_PLANE__H__

#include <sgct/config.h>
#include <glm/glm.hpp>

namespace tinyxml2 { class XMLElement; }

namespace sgct::core {

/**
 * This class holds and manages the 3D projection plane
 */
class ProjectionPlane {
public:
    void reset();
    void offset(const glm::vec3& p);

    void setCoordinateLowerLeft(glm::vec3 coordinate);
    void setCoordinateUpperLeft(glm::vec3 coordinate);
    void setCoordinateUpperRight(glm::vec3 coordinate);

    /// \returns coordinates for the lower left projection plane corner
    glm::vec3 getCoordinateLowerLeft() const;

    /// \returns coordinates for the upper left projection plane corner
    glm::vec3 getCoordinateUpperLeft() const;

    /// \returns coordinates for the upper right projection plane corner
    glm::vec3 getCoordinateUpperRight() const;

protected:
    struct {
        glm::vec3 lowerLeft = glm::vec3(-1.f, -1.f, -2.f);
        glm::vec3 upperLeft = glm::vec3(-1.f, 1.f, -2.f);
        glm::vec3 upperRight = glm::vec3(1.f, 1.f, -2.f);
    } mPlaneCoords;
};

} // namespace sgct::core

#endif // __SGCT__PROJECTION_PLANE__H__
