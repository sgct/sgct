/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__SPHERE__H__
#define __SGCT__SPHERE__H__

namespace sgct::utils {

/**
 * This class creates and renders a textured sphere.
 */
class Sphere {
public:
    /// This constructor requires a valid openGL contex
    Sphere(float radius, unsigned int segments);
    ~Sphere();

    /**
     * layout 0 contains texture coordinates (vec2)
     * layout 1 contains vertex normals (vec3)
     * layout 2 contains vertex positions (vec3).
     */
    void draw();

private:
    unsigned int _nVertices = 0;
    unsigned int _nFaces = 0;

    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    unsigned int _ibo = 0;
};

} // namespace sgct::utils

#endif // __SGCT__SPHERE__H__
