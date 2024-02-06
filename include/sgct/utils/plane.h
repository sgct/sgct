/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__PLANE__H__
#define __SGCT__PLANE__H__

#include <sgct/sgctexports.h>

namespace sgct::utils {

/**
 * This class creates and renders a textured box.
 */
class SGCT_EXPORT Plane {
public:
    /**
     * This constructor requires a valid OpenGL contex.
     */
    Plane(float width, float height);
    ~Plane();

    void draw();

private:
    unsigned int _vao = 0;
    unsigned int _vbo = 0;
};

} // namespace sgct::utils

#endif // __SGCT__PLANE__H__
