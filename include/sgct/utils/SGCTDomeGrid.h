/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__DOME_GRID__H__
#define __SGCT__DOME_GRID__H__

namespace sgct_utils {

/**
 * Helper class to render a dome grid
 */
class SGCTDomeGrid {
public:
    /// This constructor requires a valid openGL contex
    SGCTDomeGrid(float radius, float FOV, unsigned int segments, unsigned int rings,
        unsigned int resolution = 128);
    ~SGCTDomeGrid();

    /// If openGL 3.3+ is used layout 0 contains vertex positions (vec3).
    void draw();

private:
    void drawVBO();
    void drawVAO();

    void createVBO(float radius, float FOV);

private:
    unsigned int mResolution;
    unsigned int mRings;
    unsigned int mSegments;
    unsigned int mVBO = 0;
    unsigned int mVAO = 0;
};

} // namespace sgct_utils

#endif // __SGCT__DOME_GRID__H__
