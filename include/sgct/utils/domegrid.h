/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__DOMEGRID__H__
#define __SGCT__DOMEGRID__H__

#include <sgct/sgctexports.h>

namespace sgct::utils {

/**
 * Helper class to render a dome grid.
 */
class SGCT_EXPORT DomeGrid {
public:
    /**
     * This constructor requires a valid OpenGL contex.
     */
    DomeGrid(float radius, float FOV, int segments, int rings, int resolution = 128);

    /**
     * The destructor requires a valid OpenGL context.
     */
    ~DomeGrid();

    void draw();

private:
    const int _resolution;
    const int _rings;
    const int _segments;
    unsigned int _vao = 0;
    unsigned int _vbo = 0;
};

} // namespace sgct::utils

#endif // __SGCT__DOMEGRID__H__
