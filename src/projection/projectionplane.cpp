/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection/projectionplane.h>

#include <utility>

// @TODO (abock, 2019-10-15) There seems to be an issue with the rendering of the
// z coordinate of the place is 0 even if the user position is not a zero

namespace sgct {

void ProjectionPlane::offset(const vec3& p) {
    _lowerLeft.x += p.x;
    _lowerLeft.y += p.y;
    _lowerLeft.z += p.z;

    _upperLeft.x += p.x;
    _upperLeft.y += p.y;
    _upperLeft.z += p.z;

    _upperRight.x += p.x;
    _upperRight.y += p.y;
    _upperRight.z += p.z;
}

void ProjectionPlane::setCoordinates(vec3 lowerLeft, vec3 upperLeft, vec3 upperRight) {
    _lowerLeft = std::move(lowerLeft);
    _upperLeft = std::move(upperLeft);
    _upperRight = std::move(upperRight);
}

const vec3& ProjectionPlane::coordinateLowerLeft() const {
    return _lowerLeft;
}

const vec3& ProjectionPlane::coordinateUpperLeft() const {
    return _upperLeft;
}

const vec3& ProjectionPlane::coordinateUpperRight() const {
    return _upperRight;
}

} // namespace sgct
