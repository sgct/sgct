/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/projectionplane.h>

namespace sgct::core {

void ProjectionPlane::reset() {
    _lowerLeft = glm::vec3(-1.f, -1.f, -2.f);
    _upperLeft = glm::vec3(-1.f, 1.f, -2.f);
    _upperRight = glm::vec3(1.f, 1.f, -2.f);
}

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

glm::vec3 ProjectionPlane::getCoordinateLowerLeft() const {
    return _lowerLeft;
}

glm::vec3 ProjectionPlane::getCoordinateUpperLeft() const {
    return _upperLeft;
}

glm::vec3 ProjectionPlane::getCoordinateUpperRight() const {
    return _upperRight;
}

} // namespace sgct::core
