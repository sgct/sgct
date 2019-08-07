/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__DOME__H__
#define __SGCT__DOME__H__

#include "../helpers/SGCTVertexData.h"
#include <vector>

namespace sgct_utils {

/*!
    Helper class to render a dome grid
*/
class SGCTDome {
public:
    SGCTDome(float radius, float FOV, unsigned int azimuthSteps,
        unsigned int elevationSteps);
    ~SGCTDome();
    void draw();

private:
    void drawVBO();
    void drawVAO();

    void createVBO();
    void cleanup();

private:
    std::vector<sgct_helpers::SGCTVertexData> mVerts;
    std::vector<unsigned int> mIndices;
    int mElevationSteps;
    int mAzimuthSteps;

    enum bufferType { Vertex = 0, Index };
    unsigned int mVBO[2] = { 0, 0 };
    unsigned int mVAO = 0;
};

} // namespace sgct_utils

#endif // __SGCT__DOME__H__
