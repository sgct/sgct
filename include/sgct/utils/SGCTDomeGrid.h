/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__DOME_GRID__H__
#define __SGCT__DOME_GRID__H__

namespace sgct_utils {

/*!
    Helper class to render a dome grid
*/
class SGCTDomeGrid {
public:
    SGCTDomeGrid(float radius, float FOV, unsigned int segments, unsigned int rings,
        unsigned int resolution = 128);
    ~SGCTDomeGrid();
    void draw();

private:
    void drawVBO();
    void drawVAO();

    void createVBO();
    void cleanup();

private:
    float* mVerts = nullptr;
    unsigned int mNumberOfVertices;
    unsigned int mResolution;
    unsigned int mRings;
    unsigned int mSegments;
    unsigned int mVBO = 0;
    unsigned int mVAO = 0;
};

} // namespace sgct_utils

#endif // __SGCT__DOME_GRID__H__
