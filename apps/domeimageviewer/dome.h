/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__DOME__H__
#define __SGCT__DOME__H__

#include <sgct/sgctexports.h>

/**
 * Helper class to render a dome grid.
 */
class Dome {
public:
    /**
     * This constructor requires a valid OpenGL context.
     */
    Dome(float r, float FOV, unsigned int azimuthSteps, unsigned int elevationSteps);

    /**
     * The destructor requires a valid OpenGL context.
     */
    ~Dome();

    void draw() const;

private:
    const int _elevationSteps;
    const int _azimuthSteps;

    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    unsigned int _ibo = 0;
};

#endif // __SGCT__DOME__H__
