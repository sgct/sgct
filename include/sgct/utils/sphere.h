/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__SPHERE__H__
#define __SGCT__SPHERE__H__

#include <sgct/sgctexports.h>

namespace sgct::utils {

/**
 * This class creates and renders a textured sphere.
 */
class SGCT_EXPORT Sphere {
public:
    /**
     * This constructor requires a valid OpenGL context.
     */
    Sphere(float radius, unsigned int segments);

    /**
     * The destructor requires a valid OpenGL context.
     */
    ~Sphere();

    void draw();

private:
    unsigned int _nFaces = 0;

    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    unsigned int _ibo = 0;
};

} // namespace sgct::utils

#endif // __SGCT__SPHERE__H__
