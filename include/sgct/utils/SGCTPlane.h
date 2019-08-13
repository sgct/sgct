/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__PLANE__H__
#define __SGCT__PLANE__H__

namespace sgct_helpers {
struct SGCTVertexData;
} // namespace sgct_helpers

/*! \namespace sgct_utils
\brief SGCT utils namespace contains basic utilities for geometry rendering
*/
namespace sgct_utils {

/*!
    This class creates and renders a textured box.
*/
class SGCTPlane {
public:
    SGCTPlane(float width, float height);
    ~SGCTPlane();
    void draw();

private:
    void drawVBO();
    void drawVAO();

    void cleanUp();
    void createVBO();

private:    
    unsigned int mVBO = 0;
    unsigned int mVAO = 0;
    sgct_helpers::SGCTVertexData* mVerts = nullptr;
};

} // namespace sgct_utils

#endif // __SGCT__PLANE__H__
