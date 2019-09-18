/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__PLANE__H__
#define __SGCT__PLANE__H__

/**
 * \namespace sgct_utils
 * \brief SGCT utils namespace contains basic utilities for geometry rendering
*/
namespace sgct_utils {

/**
 * This class creates and renders a textured box.
*/
class Plane {
public:
    /// This constructor requires a valid OpenGL contex 
    Plane(float width, float height);
    ~Plane();

    /**
     * If openGL 3.3+ is used:
     *   layout 0 contains texture coordinates (vec2)
     *   layout 1 contains vertex normals (vec3)
     *   layout 2 contains vertex positions (vec3).
     */
    void draw();

private:
    void drawVBO();
    void drawVAO();

    void createVBO(float width, float height);

private:    
    unsigned int mVBO = 0;
    unsigned int mVAO = 0;
};

} // namespace sgct_utils

#endif // __SGCT__PLANE__H__
