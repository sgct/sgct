/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__DOMEGRID__H__
#define __SGCT__DOMEGRID__H__

/**
 * Helper class to render a dome grid.
 */
class DomeGrid {
public:
    /**
     * This constructor requires a valid OpenGL contex.
     */
    DomeGrid(float radius, float FOV, int segments, int rings, int resolution = 128);

    /**
     * The destructor requires a valid OpenGL context.
     */
    ~DomeGrid();

    void draw() const;

private:
    const int _resolution;
    const int _rings;
    const int _segments;
    unsigned int _vao = 0;
    unsigned int _vbo = 0;
};

#endif // __SGCT__DOMEGRID__H__
