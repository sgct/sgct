/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__BUFFER__H__
#define __SGCT__BUFFER__H__

#include <sgct/sgctexports.h>
#include <vector>

namespace sgct::correction {

struct SGCT_EXPORT Buffer {
    /**
     * This struct represents an individual vertex that is part of a \see Buffer.
     */
    struct Vertex {
        float x = 0.f;
        float y = 0.f;
        float s = 0.f;
        float t = 0.f;
        float r = 0.f;
        float g = 0.f;
        float b = 0.f;
        float a = 0.f;
    };

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int geometryType = 0x0004; // = GL_TRIANGLES
};

} // namespace sgct::correction

#endif // __SGCT__BUFFER__H__
