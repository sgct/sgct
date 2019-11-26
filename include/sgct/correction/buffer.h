/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__BUFFER__H__
#define __SGCT__BUFFER__H__

#include <sgct/ogl_headers.h>
#include <vector>

namespace sgct::core::correction {

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
    GLenum geometryType = GL_TRIANGLES;
};

} // namespace sgct::core::correction

#endif // __SGCT__BUFFER__H__
