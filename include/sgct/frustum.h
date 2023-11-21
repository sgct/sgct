/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__FRUSTUM__H__
#define __SGCT__FRUSTUM__H__

#include <sgct/sgctexports.h>

namespace sgct {

/**
 * The frustum class stores the view frustum for each viewport.
 */
struct SGCT_EXPORT Frustum {
    /**
     * SGCT stores all view frustums as triplets for switching between mono and stereo.
     */
    enum class Mode { MonoEye = 0, StereoLeftEye, StereoRightEye };

    float left = -1.f;
    float right = 1.f;
    float bottom = -1.f;
    float top = 1.f;
    float nearPlane = 0.1f;
    float farPlane = 100.f;
};

} // namespace sgct

#endif // __SGCT__FRUSTUM__H__
