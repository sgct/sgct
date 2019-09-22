/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__VIEWPORT__H__
#define __SGCT__VIEWPORT__H__

#include <sgct/baseviewport.h>

#include <sgct/config.h>
#include <sgct/correctionmesh.h>
#include <memory>
#include <string>
#include <vector>

#define TIXML_USE_STL // needed for tinyXML lib to link properly in mingw
#include <tinyxml2.h>

namespace sgct::core {

class NonLinearProjection;

/**
 * This class holds and manages viewportdata and calculates frustums
 */
class Viewport : public BaseViewport {
public:
    Viewport();

    /// Create a viewport coordinates are relative to the window size [0, 1]
    Viewport(float x, float y, float xSize, float ySize);

    /// Destructor that deletes any overlay or mask textures
    ~Viewport();

    void configure(tinyxml2::XMLElement* element);
    void configureMpcdi(tinyxml2::XMLElement* element);
    void setOverlayTexture(std::string texturePath);
    void setBlendMaskTexture(std::string texturePath);
    void setBlackLevelMaskTexture(std::string texturePath);
    void setCorrectionMesh(std::string meshPath);
    void setMpcdiWarpMesh(std::vector<unsigned char> data);
    void setTracked(bool state);
    void loadData();

    /// Render the viewport mesh which the framebuffer texture is attached to
    void renderQuadMesh() const;

    /// Render the viewport mesh which the framebuffer texture is attached to
    void renderWarpMesh() const;

    /// Render the viewport mesh which the framebuffer texture is attached to
    void renderMaskMesh() const;

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
    NonLinearProjection* getNonLinearProjection() const;
    const std::vector<unsigned char>& mpcdiWarpMesh() const;

private:
    void applyPlanarProjection(const sgct::config::PlanarProjection& proj);
    void applyFisheyeProjection(const sgct::config::FisheyeProjection& proj);
    void applySpoutOutputProjection(const sgct::config::SpoutOutputProjection& proj);
    void applySphericalMirrorProjection(const sgct::config::SphericalMirrorProjection& proj);

    CorrectionMesh mCM;
    std::string mOverlayFilename;
    std::string mBlendMaskFilename;
    std::string mBlackLevelMaskFilename;
    std::string mMeshFilename;
    std::string mMeshHint;
    bool mCorrectionMesh = false;
    bool mTracked = false;
    unsigned int mOverlayTextureIndex = 0;
    unsigned int mBlendMaskTextureIndex = 0;
    unsigned int mBlackLevelMaskTextureIndex = 0;

    std::unique_ptr<NonLinearProjection> mNonLinearProjection;
    std::vector<unsigned char> mMpcdiWarpMesh;
};

} // namespace sgct::core

#endif // __SGCT__VIEWPORT__H__
