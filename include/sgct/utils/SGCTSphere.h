/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__SPHERE__H__
#define __SGCT__SPHERE__H__

namespace sgct_helpers {
struct SGCTVertexData;
} // namespace sgct_helpers

namespace sgct_utils {

/*!
    This class creates and renders a textured sphere.
*/
class SGCTSphere {
public:
    SGCTSphere(float radius, unsigned int segments);
    ~SGCTSphere();
    void draw();

private:
    void addVertexData(unsigned int pos, float t, float s, float nx, float ny, float nz,
        float x, float y, float z);

    void drawVBO();
    void drawVAO();

    void createVBO();
    void cleanUp();

    sgct_helpers::SGCTVertexData* mVerts = nullptr;
    unsigned int* mIndices = nullptr;

    unsigned int mNumberOfVertices;
    unsigned int mNumberOfFaces;

    enum BufferType { Vertex = 0, Index };
    unsigned int mVBO[2] = { 0, 0 };
    unsigned int mVAO = 0;
};

} // namespace sgct_utils

#endif // __SGCT__SPHERE__H__
