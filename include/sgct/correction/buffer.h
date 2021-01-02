/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2021                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__BUFFER__H__
#define __SGCT__BUFFER__H__

#include <vector>

namespace sgct::correction {

struct CorrectionMeshVertex {
    float x = 0.f;
    float y = 0.f;
    float s = 0.f;
    float t = 0.f;
    float r = 0.f;
    float g = 0.f;
    float b = 0.f;
    float a = 0.f;
};

struct Buffer {
    std::vector<CorrectionMeshVertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int geometryType = 0x0004; // = GL_TRIANGLES
};

} // namespace sgct::correction

#endif // __SGCT__BUFFER__H__
