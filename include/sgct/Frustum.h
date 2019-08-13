/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__FRUSTUM__H__
#define __SGCT__FRUSTUM__H__

namespace sgct_core {

/*!
    The frustum class stores the view frustum for each viewport
*/
struct Frustum {
    /*! 
        SGCT stores all view frustums as triplets for easy switching between mono and stereo
    */
    enum FrustumMode { MonoEye = 0, StereoLeftEye, StereoRightEye };

    float left = -1.f;
    float right = 1.f;
    float bottom = -1.f;
    float top = 1.f;
    float nearPlane = 0.1f;
    float farPlane = 100.f;
};

} // namespace sgct_core

#endif // __SGCT__FRUSTUM__H__
