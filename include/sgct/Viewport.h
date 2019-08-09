/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__VIEWPORT__H__
#define __SGCT__VIEWPORT__H__

#include <sgct/BaseViewport.h>
#include <sgct/NonLinearProjection.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <sgct/CorrectionMesh.h>
#include <stddef.h> //get definition for NULL

#define TIXML_USE_STL //needed for tinyXML lib to link properly in mingw
#ifndef SGCT_DONT_USE_EXTERNAL
    #include <external/tinyxml2.h>
#else
    #include <tinyxml2.h>
#endif
#define MAX_XML_DEPTH 16


namespace sgct_core {
struct FrustumData {
    enum elemIdx {
        down = 0,
        up,
        left,
        right,
        yaw,
        pitch,
        roll,
        numElements //This must be last entry
    };
    bool foundElem[numElements];
    float value[numElements];

    FrustumData() {
        for (bool& i : foundElem)
            i = false;
        for (float& i : value)
            i = 0.0;
    };
};
/*!
    This class holds and manages viewportdata and calculates frustums
*/
class Viewport : public BaseViewport
{
public:
    Viewport(float x = 0.f, float y = 0.f, float xSize = 1.f, float ySize = 1.f);
    ~Viewport();

    void configure(tinyxml2::XMLElement* element);
    void configureMpcdi(tinyxml2::XMLElement* element[], const char* val[], int winResX,
        int winResY);
    void setOverlayTexture(const char* texturePath);
    void setBlendMaskTexture(const char* texturePath);
    void setBlackLevelMaskTexture(const char* texturePath);
    void setCorrectionMesh(const char* meshPath);
    void setMpcdiWarpMesh(const char* meshData, size_t size);
    void setTracked(bool state);
    void loadData();

    void renderMesh(CorrectionMesh::MeshType mt);

    bool hasOverlayTexture();
    bool hasBlendMaskTexture();
    bool hasBlackLevelMaskTexture();
    bool hasSubViewports();
    bool hasCorrectionMesh();
    bool isTracked();
    unsigned int getOverlayTextureIndex();
    unsigned int getBlendMaskTextureIndex();
    unsigned int getBlackLevelMaskTextureIndex();
    CorrectionMesh* getCorrectionMeshPtr();
    NonLinearProjection* getNonLinearProjectionPtr();

    char* mMpcdiWarpMeshData = nullptr;
    size_t mMpcdiWarpMeshSize = 0;

private:
    //void reset(float x, float y, float xSize, float ySize);
    void parsePlanarProjection(tinyxml2::XMLElement* element);
    void parseFisheyeProjection(tinyxml2::XMLElement* element);
    void parseSpoutOutputProjection(tinyxml2::XMLElement* element);
    void parseSphericalMirrorProjection(tinyxml2::XMLElement* element);
    void parseFloatFromAttribute(tinyxml2::XMLElement* element, const std::string& tag,
        float& target);
    bool parseFrustumElement(FrustumData& frustum, FrustumData::elemIdx elemIndex,
        tinyxml2::XMLElement* elem, const char* frustumTag);

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

    NonLinearProjection* mNonLinearProjection = nullptr;
};

} // namespace sgct_core

#endif // __SGCT__VIEWPORT__H__
