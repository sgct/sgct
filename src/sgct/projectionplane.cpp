/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/projectionplane.h>

// @TODO (abock, 2019-10-15) There seems to be an issue with the rendering of the 
// z coordinate of the place is 0 even if the user position is not a zero

namespace sgct {

void ProjectionPlane::offset(const glm::vec3& p) {
    _lowerLeft += p;
    _upperLeft += p;
    _upperRight += p;
}

void ProjectionPlane::setCoordinates(glm::vec3 lowerLeft, glm::vec3 upperLeft,
                                     glm::vec3 upperRight)
{
    _lowerLeft = std::move(lowerLeft);
    _upperLeft = std::move(upperLeft);
    _upperRight = std::move(upperRight);
}

const glm::vec3& ProjectionPlane::coordinateLowerLeft() const {
    return _lowerLeft;
}

const glm::vec3& ProjectionPlane::coordinateUpperLeft() const {
    return _upperLeft;
}

const glm::vec3& ProjectionPlane::coordinateUpperRight() const {
    return _upperRight;
}

} // namespace sgct
