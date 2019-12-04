/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__PROJECTIONPLANE__H__
#define __SGCT__PROJECTIONPLANE__H__

#include <glm/glm.hpp>

namespace sgct::core {

/// This class holds and manages the 3D projection plane
class ProjectionPlane {
public:
    void setCoordinates(glm::vec3 lowerLeft, glm::vec3 upperLeft, glm::vec3 upperRight);
    void offset(const glm::vec3& p);

    /// \return coordinates for the lower left projection plane corner
    const glm::vec3& coordinateLowerLeft() const;

    /// \return coordinates for the upper left projection plane corner
    const glm::vec3& coordinateUpperLeft() const;

    /// \return coordinates for the upper right projection plane corner
    const glm::vec3& coordinateUpperRight() const;

private:
    glm::vec3 _lowerLeft = glm::vec3(-1.f, -1.f, -2.f);
    glm::vec3 _upperLeft = glm::vec3(-1.f, 1.f, -2.f);
    glm::vec3 _upperRight = glm::vec3(1.f, 1.f, -2.f);
};

} // namespace sgct::core

#endif // __SGCT__PROJECTIONPLANE__H__
