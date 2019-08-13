/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__VIEWPORT__H__
#define __SGCT__VIEWPORT__H__

#include <sgct/BaseViewport.h>

#include <sgct/CorrectionMesh.h>
#include <memory>
#include <string>
#include <vector>

#define TIXML_USE_STL //needed for tinyXML lib to link properly in mingw
#ifndef SGCT_DONT_USE_EXTERNAL
    #include <external/tinyxml2.h>
#else
    #include <tinyxml2.h>
#endif

namespace sgct_core {

class NonLinearProjection;

/*!
    This class holds and manages viewportdata and calculates frustums
*/
class Viewport : public BaseViewport {
public:
    Viewport();
    Viewport(float x, float y, float xSize, float ySize);
    ~Viewport();

    void configure(tinyxml2::XMLElement* element);
    void configureMpcdi(tinyxml2::XMLElement* element[], const char* val[], int winResX,
        int winResY);
    void setOverlayTexture(std::string texturePath);
    void setBlendMaskTexture(std::string texturePath);
    void setBlackLevelMaskTexture(std::string texturePath);
    void setCorrectionMesh(std::string meshPath);
    void setMpcdiWarpMesh(std::vector<unsigned char> data);
    void setTracked(bool state);
    void loadData();

    void renderMesh(CorrectionMesh::MeshType mt);

    bool hasOverlayTexture() const;
    bool hasBlendMaskTexture() const;
    bool hasBlackLevelMaskTexture() const;
    bool hasSubViewports() const;
    bool hasCorrectionMesh() const;
    bool isTracked() const;
    unsigned int getOverlayTextureIndex() const;
    unsigned int getBlendMaskTextureIndex() const;
    unsigned int getBlackLevelMaskTextureIndex() const;
    CorrectionMesh& getCorrectionMeshPtr();
    NonLinearProjection* getNonLinearProjectionPtr() const;
    const std::vector<unsigned char>& mpcdiWarpMesh() const;

private:
    void parsePlanarProjection(tinyxml2::XMLElement* element);
    void parseFisheyeProjection(tinyxml2::XMLElement* element);
    void parseSpoutOutputProjection(tinyxml2::XMLElement* element);
    void parseSphericalMirrorProjection(tinyxml2::XMLElement* element);

    CorrectionMesh mCM;
    std::string mOverlayFilename;
    std::string mBlendMaskFilename;
    std::string mBlackLevelMaskFilename;
    std::string mMeshFilename;
    std::string mMeshHint;
    bool mCorrectionMesh = false;
    bool mTracked = false;
    bool mIsMeshStoredInFile = false;
    unsigned int mOverlayTextureIndex = 0;
    unsigned int mBlendMaskTextureIndex = 0;
    unsigned int mBlackLevelMaskTextureIndex = 0;

    std::unique_ptr<NonLinearProjection> mNonLinearProjection;
    std::vector<unsigned char> mMpcdiWarpMesh;
};

} // namespace sgct_core

#endif // __SGCT__VIEWPORT__H__
