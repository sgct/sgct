/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__PROJECTIONPLANE__H__
#define __SGCT__PROJECTIONPLANE__H__

#include <sgct/sgctexports.h>
#include <sgct/math.h>

namespace sgct {

/**
 * This class holds and manages the 3D projection plane.
 */
class SGCT_EXPORT ProjectionPlane {
public:
    void setCoordinates(vec3 lowerLeft, vec3 upperLeft, vec3 upperRight);
    void offset(const vec3& p);

    /**
     * \return coordinates for the lower left projection plane corner
     */
    const vec3& coordinateLowerLeft() const;

    /**
     * \return coordinates for the upper left projection plane corner
     */
    const vec3& coordinateUpperLeft() const;

    /**
     * \return coordinates for the upper right projection plane corner
     */
    const vec3& coordinateUpperRight() const;

private:
    vec3 _lowerLeft = vec3{ -1.f, -1.f, -2.f };
    vec3 _upperLeft = vec3{ -1.f, 1.f, -2.f };
    vec3 _upperRight = vec3{ 1.f, 1.f, -2.f };
};

} // namespace sgct

#endif // __SGCT__PROJECTIONPLANE__H__
