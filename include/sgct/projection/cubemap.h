/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CUBEMAP__H__
#define __SGCT__CUBEMAP__H__

#include <sgct/sgctexports.h>
#include <sgct/projection/nonlinearprojection.h>

#include <sgct/callbackdata.h>
#include <array>
#include <memory>

#ifdef SGCT_HAS_SPOUT
struct SPOUTLIBRARY;
typedef SPOUTLIBRARY* SPOUTHANDLE;
#endif // SGCT_HAS_SPOUT

namespace sgct {

class OffScreenBuffer;

/**
 * This class manages and renders non-linear fisheye projections.
 */
class SGCT_EXPORT CubemapProjection final : public NonLinearProjection {
public:
    CubemapProjection(const config::CubemapProjection& config,
        const Window& parent, User& user);
    virtual ~CubemapProjection() override;

    /**
     * Update projection when aspect ratio changes for the viewport.
     */
    void update(const vec2& size) const override;

    /**
     * Render the non linear projection to currently bounded FBO.
     */
    void render(const BaseViewport& viewport, FrustumMode frustumMode) const override;

    /**
     * Render the enabled faces of the cubemap.
     */

    void setSpoutRigOrientation(vec3 orientation);

private:
    void initTextures(unsigned int internalFormat, unsigned int format,
        unsigned int type) override;
    void initVBO() override;
    void initViewports() override;
    void initShaders() override;
    void initFBO(unsigned int internalFormat, int nSamples) override;

    void renderCubemap(FrustumMode frustumMode) const override;

    std::unique_ptr<OffScreenBuffer> _spoutFBO;

    struct SpoutInfo {
        bool enabled;
#ifdef SGCT_HAS_SPOUT
        SPOUTHANDLE handle;
#endif // SGCT_HAS_SPOUT
        unsigned int texture;
    };
    std::array<SpoutInfo, 6> _spout;

    std::string _spoutName;
    vec3 _rigOrientation = vec3{ 0.f, 0.f, 0.f };

    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    ShaderProgram _shader;
    unsigned int _blitFbo = 0;
};

} // namespace sgct

#endif // __SGCT__CUBEMAP__H__
