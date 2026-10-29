/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
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
    /// How the indices form triangles
    enum class GeometryType { Triangles, TriangleStrip };

    std::vector<unsigned int> indices;
    GeometryType geometryType = GeometryType::Triangles;
};

} // namespace sgct::correction

#endif // __SGCT__BUFFER__H__
