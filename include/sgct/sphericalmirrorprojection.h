/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__SPHERICAL_MIRROR_PROJECTION__H__
#define __SGCT__SPHERICAL_MIRROR_PROJECTION__H__

#include <sgct/nonlinearprojection.h>

#include <sgct/correctionmesh.h>
#include <glm/glm.hpp>

namespace sgct::core {

/**
 * This class manages and renders non-linear fisheye projections
 */
class SphericalMirrorProjection : public NonLinearProjection {
public:
    SphericalMirrorProjection() = default;
    virtual ~SphericalMirrorProjection() = default;

    virtual void update(glm::vec2 size) override;

    /// Render the non linear projection to currently bounded FBO
    virtual void render() override;

    /// Render the enabled faces of the cubemap
    virtual void renderCubemap(size_t* subViewPortIndex) override;

    /**
     * Set the dome tilt angle used in the spherical mirror renderer. The tilt angle is
     * from the horizontal.
     *
     * \param angle the tilt angle in degrees
     */
    void setTilt(float angle);

    /**
     * Set the mesh path for selected cube face.
     *
     * \param mt the mesh face
     * \param str the path to the mesh
     */
    void setMeshPaths(std::string bottom, std::string left, std::string right,
        std::string top);

private:
    virtual void initTextures() override;
    virtual void initVBO() override;
    virtual void initViewports() override;
    virtual void initShaders() override;
        
    void drawCubeFace(size_t face);
    void blitCubeFace(unsigned int texture);
    void attachTextures(unsigned int texture);
    void renderInternal();
    void renderInternalFixedPipeline();
    void renderCubemapInternal(size_t* subViewPortIndex);
    void renderCubemapInternalFixedPipeline(size_t* subViewPortIndex);

    float _tilt = 0.f;
    float _diameter = 2.4f;
        
    // mesh data
    struct {
        CorrectionMesh bottom;
        CorrectionMesh left;
        CorrectionMesh right;
        CorrectionMesh top;
    } _meshes;
    struct {
        std::string bottom;
        std::string left;
        std::string right;
        std::string top;
    } _meshPaths;

    // shader locations
    int _texLoc = -1;
    int _matrixLoc = -1;
};

} // namespace sgct::core

#endif // __SGCT__SPHERICAL_MIRROR_PROJECTION__H__
