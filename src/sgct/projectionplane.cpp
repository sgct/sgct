/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/projectionplane.h>

#include <sgct/config.h>
#include <sgct/messagehandler.h>

namespace sgct::core {

void ProjectionPlane::reset() {
    mPlaneCoords.lowerLeft = glm::vec3(-1.f, -1.f, -2.f);
    mPlaneCoords.upperLeft = glm::vec3(-1.f, 1.f, -2.f);
    mPlaneCoords.upperRight = glm::vec3(1.f, 1.f, -2.f);
}

void ProjectionPlane::offset(const glm::vec3& p) {
    mPlaneCoords.lowerLeft += p;
    mPlaneCoords.upperLeft += p;
    mPlaneCoords.upperRight += p;
}

void ProjectionPlane::setCoordinateLowerLeft(glm::vec3 coordinate) {
    mPlaneCoords.lowerLeft = std::move(coordinate);
}

void ProjectionPlane::setCoordinateUpperLeft(glm::vec3 coordinate) {
    mPlaneCoords.upperLeft = std::move(coordinate);
}

void ProjectionPlane::setCoordinateUpperRight(glm::vec3 coordinate) {
    mPlaneCoords.upperRight = std::move(coordinate);
}

glm::vec3 ProjectionPlane::getCoordinateLowerLeft() const {
    return mPlaneCoords.lowerLeft;
}

glm::vec3 ProjectionPlane::getCoordinateUpperLeft() const {
    return mPlaneCoords.upperLeft;
}

glm::vec3 ProjectionPlane::getCoordinateUpperRight() const {
    return mPlaneCoords.upperRight;
}

} // namespace sgct::core
