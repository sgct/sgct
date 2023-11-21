/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__FISHEYE__H__
#define __SGCT__FISHEYE__H__

#include <sgct/sgctexports.h>
#include <sgct/projection/nonlinearprojection.h>

#include <sgct/callbackdata.h>

namespace sgct {

/**
 * This class manages and renders non linear fisheye projections.
 */
class SGCT_EXPORT FisheyeProjection final : public NonLinearProjection {
public:
    enum class FisheyeMethod { FourFaceCube = 0, FiveFaceCube, SixFaceCube };

    FisheyeProjection(const Window* parent);
    ~FisheyeProjection() final;

    /**
     * Update projection when aspect ratio changes for the viewport.
     */
    void update(vec2 size) override;

    /**
     * Render the non-linear projection to currently bounded FBO.
     */
    void render(const Window& window, const BaseViewport& viewport,
        Frustum::Mode frustumMode) override;

    /**
     * Render the enabled faces of the cubemap.
     */
    void renderCubemap(Window& window, Frustum::Mode frustumMode) override;

    /**
     * Set the dome diameter used in the fisheye renderer (used for the viewplane distance
     * calculations).
     *
     * \param diameter Diameter of the dome diameter in meters
     */
    void setDomeDiameter(float diameter);

    /**
     * Set the fisheye/dome tilt angle used in the fisheye renderer. The tilt angle is
     * from the horizontal.
     *
     * \param angle The tilt angle in degrees
     */
    void setTilt(float angle);

    /**
     * Set the fisheye/dome field-of-view angle used in the fisheye renderer.
     *
     * \param angle The FOV angle in degrees
     */
    void setFOV(float angle);

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
    void setOffset(vec3 offset);

    /**
     * Set fisheye base offset to render offaxis. Length of vector must be smaller then 1.
     * Base of fisheye is the XY-plane. The base offset will be added to the offset
     * specified by setFisheyeOffset. These values are set from the configuration.
     */
    void setBaseOffset(vec3 offset);

    /**
     * Ignore the framebuffer aspect ratio to allow non-circular fisheye. This is useful
     * for spherical mirror projections.
     */
    void setIgnoreAspectRatio(bool state);

    void setKeepAspectRatio(bool state);

private:
    void initVBO() override;
    void initViewports() override;
    void initShaders() override;

    float _fov = 180.f;
    float _tilt = 0.f;
    float _diameter = 14.8f;
    float _cropLeft = 0.f;
    float _cropRight = 0.f;
    float _cropBottom = 0.f;
    float _cropTop = 0.f;

    bool _isOffAxis = false;
    bool _ignoreAspectRatio = false;
    bool _keepAspectRatio = true;

    vec3 _offset = vec3{ 0.f, 0.f, 0.f };
    vec3 _baseOffset = vec3{ 0.f, 0.f, 0.f };
    vec3 _totalOffset = vec3{ 0.f, 0.f, 0.f };

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
    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    ShaderProgram _shader;
    ShaderProgram _depthCorrectionShader;
};

} // namespace sgct

#endif // __SGCT__FISHEYE__H__
