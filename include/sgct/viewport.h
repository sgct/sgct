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

    void applySettings(const sgct::config::Viewport& viewport);
    void applySettings(const sgct::config::MpcdiProjection& mpcdi);
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

    CorrectionMesh _mesh;
    std::string _overlayFilename;
    std::string _blendMaskFilename;
    std::string _blackLevelMaskFilename;
    std::string _meshFilename;
    std::string _meshHint;
    bool _correctionMesh = false;
    bool _tracked = false;
    unsigned int _overlayTextureIndex = 0;
    unsigned int _blendMaskTextureIndex = 0;
    unsigned int _blackLevelMaskTextureIndex = 0;

    std::unique_ptr<NonLinearProjection> _nonLinearProjection;
    std::vector<unsigned char> _mpcdiWarpMesh;
};

} // namespace sgct::core

#endif // __SGCT__VIEWPORT__H__
