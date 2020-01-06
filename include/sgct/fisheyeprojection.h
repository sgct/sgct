/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__FISHEYEPROJECTION__H__
#define __SGCT__FISHEYEPROJECTION__H__

#include <sgct/nonlinearprojection.h>

#include <sgct/callbackdata.h>
#include <glm/glm.hpp>

namespace sgct {

/// This class manages and renders non linear fisheye projections
class FisheyeProjection : public NonLinearProjection {
public:
    enum class FisheyeMethod { FourFaceCube = 0, FiveFaceCube, SixFaceCube };

    FisheyeProjection(const Window* parent);

    /// Update projection when aspect ratio changes for the viewport.
    void update(glm::vec2 size) override;

    /// Render the non-linear projection to currently bounded FBO
    void render(const Window& window, const BaseViewport& viewport,
        Frustum::Mode frustumMode) override;

    /// Render the enabled faces of the cubemap
    void renderCubemap(Window& window, Frustum::Mode frustumMode) override;

    /**
     * Set the dome diameter used in the fisheye renderer (used for the viewplane distance
     * calculations)
     *
     * \param diameter diameter of the dome diameter in meters
     */
    void setDomeDiameter(float diameter);

    /**
     * Set the fisheye/dome tilt angle used in the fisheye renderer. The tilt angle is
     * from the horizontal.
     *
     * \param angle the tilt angle in degrees
     */
    void setTilt(float angle);

    /**
     * Set the fisheye/dome field-of-view angle used in the fisheye renderer.
     *
     * \param angle the FOV angle in degrees
     */
    void setFOV(float angle);

    /**
     * Set the method used for rendering the fisheye projection.
     *
     * \param method the selected method
     */
    void setRenderingMethod(FisheyeMethod method);

    /**
     * Set the fisheye crop values. Theese values are used when rendering content for a
     * single projector dome. The elumenati geodome has usually a 4:3 SXGA+ (1400x1050)
     * projector and the fisheye is cropped 25% (350 pixels) at the top.
     */
    void setCropFactors(float left, float right, float bottom, float top);

    /**
     * Set fisheye offset to render offaxis. Length of vector must be smaller then 1.
     * Base of fisheye is the XY-plane. This function is normally used in fisheye stereo
     * rendering.
     */
    void setOffset(glm::vec3 offset);

    /**
     * Set fisheye base offset to render offaxis. Length of vector must be smaller then 1.
     * Base of fisheye is the XY-plane. The base offset will be added to the offset
     * specified by setFisheyeOffset. These values are set from the XML config.
     */
    void setBaseOffset(glm::vec3 offset);

    /**
     * Ignore the framebuffer aspect ratio to allow non-circular fisheye. This is useful
     * for spherical mirror projections.
     */
    void setIgnoreAspectRatio(bool state);

private:
    void initViewports() override;
    void initShaders() override;

    void drawCubeFace(BaseViewport& face, RenderData renderData);
    void blitCubeFace(int face);
    void attachTextures(int face);

    float _fov = 180.f;
    float _tilt = 0.f;
    float _diameter = 14.8f;
    float _cropLeft = 0.f;
    float _cropRight = 0.f;
    float _cropBottom = 0.f;
    float _cropTop = 0.f;

    bool _isOffAxis = false;
    bool _ignoreAspectRatio = false;
        
    glm::vec3 _offset = glm::vec3(0.f);
    glm::vec3 _baseOffset = glm::vec3(0.f);
    glm::vec3 _totalOffset = _baseOffset + _offset;

    FisheyeMethod _method = FisheyeMethod::FourFaceCube;

    // shader locations
    struct {
        int cubemap = -1;
        int depthCubemap = -1;
        int normalCubemap = -1;
        int positionCubemap = -1;
        int halfFov = -1;
        int offset = -1;
        int swapColor = -1;
        int swapDepth = -1;
        int swapNear = -1;
        int swapFar = -1;
    } _shaderLoc;
};

} // namespace sgct

#endif // __SGCT__FISHEYEPROJECTION__H__
