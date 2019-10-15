/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__DOME__H__
#define __SGCT__DOME__H__

#include <sgct/helpers/vertexdata.h>
#include <vector>

namespace sgct::utils {

/**
 * Helper class to render a dome grid
 */
class Dome {
public:
    /// This constructor requires a valid OpenGL contex
    Dome(float radius, float FOV, unsigned int azimuthSteps, unsigned int elevationSteps);
    ~Dome();

    /**
     * layout 0 contains texture coordinates (vec2)
     * layout 1 contains vertex normals (vec3)
     * layout 2 contains vertex positions (vec3).
     */
    void draw();

private:
    void createVBO(float radius, float FOV);

    int _elevationSteps;
    int _azimuthSteps;

    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    unsigned int _ibo = 0;
};

} // namespace sgct::utils

#endif // __SGCT__DOME__H__
