/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _VIEWPORT_H
#define _VIEWPORT_H

#include "BaseViewport.h"
#include "NonLinearProjection.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include "CorrectionMesh.h"
#include <stddef.h> //get definition for NULL
#include "unzip.h"
#include <zip.h>
#define TIXML_USE_STL //needed for tinyXML lib to link properly in mingw
#ifndef SGCT_DONT_USE_EXTERNAL
    #include <external/tinyxml2.h>
#else
    #include <tinyxml2.h>
#endif
#define MAX_XML_DEPTH 16


namespace sgct_core
{


/*!
    This class holds and manages viewportdata and calculates frustums
*/
class Viewport : public BaseViewport
{
public:
    Viewport();
    Viewport(float x, float y, float xSize, float ySize);
    ~Viewport();

    void configure(tinyxml2::XMLElement * element);
    void configureMpcdi(tinyxml2::XMLElement * element[], const char* val[], int winResX, int winResY);
    void setOverlayTexture(const char * texturePath);
    void setBlendMaskTexture(const char * texturePath);
    void setBlackLevelMaskTexture(const char * texturePath);
    void setCorrectionMesh(const char * meshPath);
    void setMpcdiWarpMesh(const char* meshData, size_t size);
    void setTracked(bool state);
    void loadData();

    void renderMesh(CorrectionMesh::MeshType mt);

    inline bool hasOverlayTexture() { return mOverlayTextureIndex != GL_FALSE; }
    inline bool hasBlendMaskTexture() { return mBlendMaskTextureIndex != GL_FALSE; }
    inline bool hasBlackLevelMaskTexture() { return mBlackLevelMaskTextureIndex != GL_FALSE; }
    inline bool hasSubViewports() { return mNonLinearProjection != NULL; }

    inline const bool & hasCorrectionMesh() { return mCorrectionMesh; }
    inline const bool & isTracked() { return mTracked; }
    inline const unsigned int & getOverlayTextureIndex() { return mOverlayTextureIndex; }
    inline const unsigned int & getBlendMaskTextureIndex() { return mBlendMaskTextureIndex; }
    inline const unsigned int & getBlackLevelMaskTextureIndex() { return mBlackLevelMaskTextureIndex; }
    inline CorrectionMesh * getCorrectionMeshPtr() { return &mCM; }
    inline NonLinearProjection * getNonLinearProjectionPtr() { return mNonLinearProjection; }

    char* mMpcdiWarpMeshData;
    size_t mMpcdiWarpMeshSize = 0;

private:
    void reset(float x, float y, float xSize, float ySize);
    void parsePlanarProjection(tinyxml2::XMLElement * element);
    void parseFisheyeProjection(tinyxml2::XMLElement * element);
    void parseSphericalMirrorProjection(tinyxml2::XMLElement * element);

private:
    CorrectionMesh mCM;
    std::string mOverlayFilename;
    std::string mBlendMaskFilename;
    std::string mBlackLevelMaskFilename;
    std::string mMeshFilename;
    std::string mMeshHint;
    bool mCorrectionMesh;
    bool mTracked;
    bool mIsMeshStoredInFile = false;
    unsigned int mOverlayTextureIndex;
    unsigned int mBlendMaskTextureIndex;
    unsigned int mBlackLevelMaskTextureIndex;

    NonLinearProjection * mNonLinearProjection;
};

}

#endif
