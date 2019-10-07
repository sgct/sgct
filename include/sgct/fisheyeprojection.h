/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__FISHEYE_PROJECTION__H__
#define __SGCT__FISHEYE_PROJECTION__H__

#include <sgct/nonlinearprojection.h>

#include <glm/glm.hpp>

namespace sgct::core {

/**
 * This class manages and renders non linear fisheye projections
 */
class FisheyeProjection : public NonLinearProjection {
public:
    enum class FisheyeMethod { FourFaceCube = 0, FiveFaceCube, SixFaceCube };

    /// Update projection when aspect ratio changes for the viewport.
    void update(glm::vec2 size) override;

    /// Render the non linear projection to currently bounded FBO
    void render() override;

    /// Render the enabled faces of the cubemap
    void renderCubemap(size_t* subViewPortIndex) override;

    /**
     * Set the dome diameter used in the fisheye renderer (used for the viewplane distance
     * calculations)
     *
     * \param diameter diameter of the dome diameter in meters
     */
    void setDomeDiameter(float size);

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

    /// Get the lens offset for off-axis projection.
    glm::vec3 getOffset() const;

private:
    void initViewports() override;
    void initShaders() override;

    void drawCubeFace(BaseViewport& face);
    void blitCubeFace(int face);
    void attachTextures(int face);
    void renderInternal();
    void renderInternalFixedPipeline();
    void renderCubemapInternal(size_t* subViewPortIndex);
    void renderCubemapInternalFixedPipeline(size_t* subViewPortIndex);

    float _fov = 180.f;
    float _tilt = 0.f;
    float _diameter = 14.8f;
    struct {
        float left = 0.f;
        float right = 0.f;
        float bottom = 0.f;
        float top = 0.f;
    } _cropFactor;

    bool _offAxis = false;
    bool _ignoreAspectRatio = false;
        
    glm::vec3 _offset = glm::vec3(0.f);
    glm::vec3 _baseOffset = glm::vec3(0.f);
    glm::vec3 _totalOffset = _baseOffset + _offset;

    FisheyeMethod _method = FisheyeMethod::FourFaceCube;

    // shader locations
    struct {
        int cubemapLoc = -1;
        int depthCubemapLoc = -1;
        int normalCubemapLoc = -1;
        int positionCubemapLoc = -1;
        int halfFovLoc = -1;
        int offsetLoc = -1;
        int swapColorLoc = -1;
        int swapDepthLoc = -1;
        int swapNearLoc = -1;
        int swapFarLoc = -1;
    } _shaderLoc;
};

} // namespace sgct::core

#endif // __SGCT__FISHEYE_PROJECTION__H__
