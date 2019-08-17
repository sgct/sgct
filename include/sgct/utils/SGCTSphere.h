/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__SPHERE__H__
#define __SGCT__SPHERE__H__

#include <vector>

namespace sgct_helpers { struct SGCTVertexData; }

namespace sgct_utils {

/**
 * This class creates and renders a textured sphere.
 */
class SGCTSphere {
public:
    /// This constructor requires a valid openGL contex
    SGCTSphere(float radius, unsigned int segments);
    ~SGCTSphere();

    /**
     * If openGL 3.3+ is used:
     *   - layout 0 contains texture coordinates (vec2)
     *   - layout 1 contains vertex normals (vec3)
     *   - layout 2 contains vertex positions (vec3).
     */
    void draw();

private:
    void drawVBO();
    void drawVAO();

    void createVBO(const std::vector<sgct_helpers::SGCTVertexData>& verts,
        const std::vector<unsigned int>& indices);

    unsigned int mNumberOfVertices = 0;
    unsigned int mNumberOfFaces = 0;

    unsigned int mVBO = 0;
    unsigned int mIBO = 0;
    unsigned int mVAO = 0;
};

} // namespace sgct_utils

#endif // __SGCT__SPHERE__H__
