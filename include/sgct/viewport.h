/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__VIEWPORT__H__
#define __SGCT__VIEWPORT__H__

#include <sgct/sgctexports.h>
#include <sgct/baseviewport.h>
#include <sgct/correctionmesh.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace sgct::config {
    struct CylindricalProjection;
    struct EquirectangularProjection;
    struct FisheyeProjection;
    struct PlanarProjection;
    struct SphericalMirrorProjection;
    struct SpoutOutputProjection;
    struct SpoutFlatProjection;
    struct TextureMappedProjection;
    struct Viewport;
} // namespace sgct::config

namespace sgct {

class NonLinearProjection;

/**
 * This class holds and manages viewportdata and calculates frustums.
 */
class SGCT_EXPORT Viewport final : public BaseViewport {
public:
    Viewport(const config::Viewport& viewport, const Window* parent);
    ~Viewport() override;

    void initialize(vec2 size, bool hasStereo, unsigned int internalFormat,
        unsigned int format, unsigned int type, int samples);

    void loadData();

    void calculateFrustum(FrustumMode mode, float nearClip, float farClip) override;

    void renderQuadMesh() const;
    void renderWarpMesh() const;
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

private:
    CorrectionMesh _mesh;
    std::filesystem::path _overlayFilename;
    std::filesystem::path _blendMaskFilename;
    std::filesystem::path _blackLevelMaskFilename;
    std::filesystem::path _meshFilename;
    bool _isTracked;
    unsigned int _overlayTextureIndex = 0;
    unsigned int _blendMaskTextureIndex = 0;
    unsigned int _blackLevelMaskTextureIndex = 0;
    bool _useTextureMappedProjection = false;

    // @TODO (abock, 2020-01-06) This can be replace with a std::variant as we have a
    // fixed list of overloads and this would remove the virtual function calls
    std::unique_ptr<NonLinearProjection> _nonLinearProjection;
};

} // namespace sgct

#endif // __SGCT__VIEWPORT__H__
