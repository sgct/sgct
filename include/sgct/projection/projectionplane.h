/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__PROJECTIONPLANE__H__
#define __SGCT__PROJECTIONPLANE__H__

#include <sgct/sgctexports.h>

#include <sgct/glm.h>

namespace sgct {

/**
 * This class holds and manages the 3D projection plane.
 */
class SGCT_EXPORT ProjectionPlane {
public:
    void setCoordinates(glm::vec3 lowerLeft, glm::vec3 upperLeft, glm::vec3 upperRight);
    void offset(const glm::vec3& p);

    /**
     * \return Coordinates for the lower left projection plane corner
     */
    const glm::vec3& coordinateLowerLeft() const;

    /**
     * \return Coordinates for the upper left projection plane corner
     */
    const glm::vec3& coordinateUpperLeft() const;

    /**
     * \return Coordinates for the upper right projection plane corner
     */
    const glm::vec3& coordinateUpperRight() const;

private:
    glm::vec3 _lowerLeft = glm::vec3(-1.f, -1.f, -2.f);
    glm::vec3 _upperLeft = glm::vec3(-1.f, 1.f, -2.f);
    glm::vec3 _upperRight = glm::vec3(1.f, 1.f, -2.f);
};

} // namespace sgct

#endif // __SGCT__PROJECTIONPLANE__H__
