/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__PLANE__H__
#define __SGCT__PLANE__H__

/**
 * \namespace sgct::utils
 * \brief SGCT utils namespace contains basic utilities for geometry rendering
*/
namespace sgct::utils {

/**
 * This class creates and renders a textured box.
*/
class Plane {
public:
    /// This constructor requires a valid OpenGL contex 
    Plane(float width, float height);
    ~Plane();

    /**
     * layout 0 contains texture coordinates (vec2)
     * layout 1 contains vertex normals (vec3)
     * layout 2 contains vertex positions (vec3).
     */
    void draw();

private:
    void createVBO(float width, float height);

    unsigned int _vao = 0;
    unsigned int _vbo = 0;
};

} // namespace sgct::utils

#endif // __SGCT__PLANE__H__
