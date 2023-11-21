/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__VIEWPORT__H__
#define __SGCT__VIEWPORT__H__

#include <sgct/sgctexports.h>
#include <sgct/baseviewport.h>
#include <sgct/correctionmesh.h>
#include <memory>
#include <string>
#include <vector>

namespace sgct::config {
    struct CylindricalProjection;
    struct EquirectangularProjection;
    struct FisheyeProjection;
    struct MpcdiProjection;
    struct PlanarProjection;
    struct SphericalMirrorProjection;
    struct SpoutOutputProjection;
    struct SpoutFlatProjection;
    struct Viewport;
} // namespace sgct::config

namespace sgct {

class NonLinearProjection;

/**
 * This class holds and manages viewportdata and calculates frustums.
 */
class SGCT_EXPORT Viewport final : public BaseViewport {
public:
    Viewport(const Window* parent);
    ~Viewport() override;

    void initialize(vec2 size, bool hasStereo, unsigned int internalFormat,
        unsigned int format, unsigned int type, int samples);

    void applyViewport(const sgct::config::Viewport& viewport);
    void applySettings(const sgct::config::MpcdiProjection& mpcdi);
    void setMpcdiWarpMesh(std::vector<char> data);
    void loadData();

    /**
     * Render the viewport mesh which the framebuffer texture is attached to.
     */
    void renderQuadMesh() const;

    /**
     * Render the viewport mesh which the framebuffer texture is attached to.
     */
    void renderWarpMesh() const;

    /**
     * Render the viewport mesh which the framebuffer texture is attached to.
     */
    void renderMaskMesh() const;

    bool hasOverlayTexture() const;
    bool hasBlendMaskTexture() const;
    bool hasBlackLevelMaskTexture() const;
    bool hasSubViewports() const;
    bool isTracked() const;
    unsigned int overlayTextureIndex() const;
    unsigned int blendMaskTextureIndex() const;
    unsigned int blackLevelMaskTextureIndex() const;
    NonLinearProjection* nonLinearProjection() const;
    const std::vector<char>& mpcdiWarpMesh() const;

private:
    void applyPlanarProjection(const config::PlanarProjection& proj);
    void applyFisheyeProjection(const config::FisheyeProjection& proj);
    void applySpoutOutputProjection(const config::SpoutOutputProjection& proj);
    void applySpoutFlatProjection(const config::SpoutFlatProjection& proj);
    void applyCylindricalProjection(const config::CylindricalProjection& proj);
    void applyEquirectangularProjection(const config::EquirectangularProjection& proj);
    void applySphericalMirrorProjection(const config::SphericalMirrorProjection& proj);

    CorrectionMesh _mesh;
    std::string _overlayFilename;
    std::string _blendMaskFilename;
    std::string _blackLevelMaskFilename;
    std::string _meshFilename;
    bool _isTracked = false;
    unsigned int _overlayTextureIndex = 0;
    unsigned int _blendMaskTextureIndex = 0;
    unsigned int _blackLevelMaskTextureIndex = 0;

    // @TODO (abock, 2020-01-06) This can be replace with a std::variant as we have a
    // fixed list of overloads and this would remove the virtual function calls
    std::unique_ptr<NonLinearProjection> _nonLinearProjection;
    std::vector<char> _mpcdiWarpMesh;
};

} // namespace sgct

#endif // __SGCT__VIEWPORT__H__
