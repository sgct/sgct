/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__SPHERICALMIRROR__H__
#define __SGCT__SPHERICALMIRROR__H__

#include <sgct/sgctexports.h>
#include <sgct/projection/nonlinearprojection.h>

#include <sgct/correctionmesh.h>
#include <memory>

namespace sgct {

namespace config { struct SphericalMirrorProjection; }
namespace vulkan { class ProjectionPass; }
class BaseViewport;
class User;
class Window;

/**
 * This class manages and renders non-linear fisheye projections.
 */
class SGCT_EXPORT SphericalMirrorProjection final : public NonLinearProjection {
public:
    SphericalMirrorProjection(const config::SphericalMirrorProjection& config,
        const Window& parent, User& user);
    ~SphericalMirrorProjection() final;

    void update(const vec2& size) const override;

    /**
     * Render the non linear projection into the viewport.
     */
    void render(VkCommandBuffer cmd, const BaseViewport& viewport,
        FrustumMode frustumMode, const VkRect2D& area) const override;

    /**
     * Set the dome tilt angle used in the spherical mirror renderer. The tilt angle is
     * from the horizontal.
     *
     * \param angle The tilt angle in degrees
     */
    void setTilt(float angle);

private:
    void initViewports() override;
    void initPipelines(VkRenderPass renderPass) override;

    float _tilt;
    float _diameter = 2.4f;

    // mesh data
    CorrectionMesh _meshBottom;
    CorrectionMesh _meshLeft;
    CorrectionMesh _meshRight;
    CorrectionMesh _meshTop;
    std::string _meshPathBottom;
    std::string _meshPathLeft;
    std::string _meshPathRight;
    std::string _meshPathTop;

    std::unique_ptr<vulkan::ProjectionPass> _pass;
};

} // namespace sgct

#endif // __SGCT__SPHERICALMIRROR__H__
