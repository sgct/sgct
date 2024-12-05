/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__SPOUTFLAT__H__
#define __SGCT__SPOUTFLAT__H__

#include <sgct/sgctexports.h>
#include <sgct/projection/nonlinearprojection.h>
#include <sgct/projection/projectionplane.h>
#include <sgct/callbackdata.h>

struct SPOUTLIBRARY;
typedef SPOUTLIBRARY* SPOUTHANDLE;

namespace sgct {

/**
 * Base class for non linear projections.
 */
class SGCT_EXPORT SpoutFlatProjection : public NonLinearProjection {
public:
    SpoutFlatProjection(const Window* parent, User* user,
        const config::SpoutFlatProjection& config);

    ~SpoutFlatProjection() override;

    void render(const BaseViewport& viewport, FrustumMode frustumMode) const override;
    void renderCubemap(FrustumMode frustumMode) const override;
    void update(const vec2& size) const override;

    void setSpoutMappingName(std::string name);
    void setResolutionWidth(int resolutionX);
    void setResolutionHeight(int resolutionY);
    void setSpoutOffset(vec3 offset);
    void setSpoutDrawMain(bool drawMain);

protected:
    void initTextures(unsigned int internalFormat, unsigned int format,
        unsigned int type) override;
    void initFBO(unsigned int internalFormat) override;
    void initVBO() override;
    void initViewports() override;
    void initShaders() override;

    void setupViewport(const BaseViewport& vp) const;
    void generateMap(unsigned int& texture, unsigned int internalFormat,
        unsigned int format, unsigned int type);

    void attachTextures(int face) const;
    void blitCubeFace(int face) const;

    struct {
        unsigned int spoutColor = 0;
        unsigned int spoutDepth = 0;
        unsigned int spoutNormals = 0;
        unsigned int spoutPositions = 0;
        unsigned int colorSwap = 0;
        unsigned int depthSwap = 0;
    } _textureIdentifiers;

    std::string _mappingName;
    SPOUTHANDLE _mappingHandle = nullptr;
    int _resolutionX = 1280;
    int _resolutionY = 720;
    ShaderProgram _shader;

    struct FOV {
        float down = 0.f;
        float left = 0.f;
        float right = 0.f;
        float up = 0.f;
        float distance = 0.f;
    };
    FOV _spoutFov;
    quat _spoutOrientation;
    vec3 _spoutOffset;
    bool _spoutDrawMain = false;

    std::unique_ptr<OffScreenBuffer> _spoutFbo;
};

} // namespace sgct

#endif // __SGCT__SPOUTFLAT__H__
