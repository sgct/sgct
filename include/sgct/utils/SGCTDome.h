/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__DOME__H__
#define __SGCT__DOME__H__

#include <sgct/helpers/SGCTVertexData.h>
#include <vector>

namespace sgct_utils {

/*!
    Helper class to render a dome grid
*/
class SGCTDome {
public:
    /// This constructor requires a valid OpenGL contex
    SGCTDome(float radius, float FOV, unsigned int azimuthSteps,
        unsigned int elevationSteps);
    ~SGCTDome();

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
    void createVBO(float radius, float FOV);

    int mElevationSteps;
    int mAzimuthSteps;

    unsigned int mVBO = 0;
    unsigned int mIBO = 0;
    unsigned int mVAO = 0;
};

} // namespace sgct_utils

#endif // __SGCT__DOME__H__
