/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/projectionplane.h>

namespace sgct::core {

void ProjectionPlane::reset() {
    lowerLeft = glm::vec3(-1.f, -1.f, -2.f);
    upperLeft = glm::vec3(-1.f, 1.f, -2.f);
    upperRight = glm::vec3(1.f, 1.f, -2.f);
}

void ProjectionPlane::offset(const glm::vec3& p) {
    lowerLeft += p;
    upperLeft += p;
    upperRight += p;
}

void ProjectionPlane::setCoordinateLowerLeft(glm::vec3 coordinate) {
    lowerLeft = std::move(coordinate);
}

void ProjectionPlane::setCoordinateUpperLeft(glm::vec3 coordinate) {
    upperLeft = std::move(coordinate);
}

void ProjectionPlane::setCoordinateUpperRight(glm::vec3 coordinate) {
    upperRight = std::move(coordinate);
}

glm::vec3 ProjectionPlane::getCoordinateLowerLeft() const {
    return lowerLeft;
}

glm::vec3 ProjectionPlane::getCoordinateUpperLeft() const {
    return upperLeft;
}

glm::vec3 ProjectionPlane::getCoordinateUpperRight() const {
    return upperRight;
}

} // namespace sgct::core
