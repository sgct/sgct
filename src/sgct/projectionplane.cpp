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

namespace sgct::core {

void ProjectionPlane::offset(const glm::vec3& p) {
    _lowerLeft += p;
    _upperLeft += p;
    _upperRight += p;
}

void ProjectionPlane::setCoordinateLowerLeft(glm::vec3 coordinate) {
    _lowerLeft = std::move(coordinate);
}

void ProjectionPlane::setCoordinateUpperLeft(glm::vec3 coordinate) {
    _upperLeft = std::move(coordinate);
}

void ProjectionPlane::setCoordinateUpperRight(glm::vec3 coordinate) {
    _upperRight = std::move(coordinate);
}

const glm::vec3& ProjectionPlane::getCoordinateLowerLeft() const {
    return _lowerLeft;
}

const glm::vec3& ProjectionPlane::getCoordinateUpperLeft() const {
    return _upperLeft;
}

const glm::vec3& ProjectionPlane::getCoordinateUpperRight() const {
    return _upperRight;
}

} // namespace sgct::core
