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
    SpoutFlatProjection(const Window* parent);

    virtual ~SpoutFlatProjection() override;

    virtual void render(const Window& window, const BaseViewport& viewport,
        Frustum::Mode frustumMode) override;
    virtual void renderCubemap(Window& window, Frustum::Mode frustumMode) override;
    virtual void update(vec2 size) override;

    void setSpoutMappingName(std::string name);
    void setResolutionWidth(int resolutionX);
    void setResolutionHeight(int resolutionY);
    void setSpoutFov(float up, float down, float left, float right, quat orientation,
        float distance);
    void setSpoutOffset(vec3 offset);
    void setSpoutDrawMain(bool drawMain);

protected:
    virtual void initTextures() override;
    virtual void initFBO() override;
    virtual void initVBO() override;
    virtual void initViewports() override;
    virtual void initShaders() override;

    void setupViewport(BaseViewport& vp);
    void generateMap(unsigned int& texture, unsigned int internalFormat,
        unsigned int format, unsigned int type);

    void attachTextures(int face);
    void blitCubeFace(int face);

    struct {
        unsigned int spoutColor = 0;
        unsigned int spoutDepth = 0;
        unsigned int spoutNormals = 0;
        unsigned int spoutPositions = 0;
        unsigned int colorSwap = 0;
        unsigned int depthSwap = 0;
    } _textureIdentifiers;

    std::string _mappingName = "SPOUT_SGCT_MAPPING";
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
    quat _spoutOrientation = quat(0.f, 0.f, 0.f, 1.f);
    vec3 _spoutOffset = vec3(0.f, 0.f, 0.f);
    bool _spoutDrawMain = false;

    std::unique_ptr<OffScreenBuffer> _spoutFbo;
};

} // namespace sgct

#endif // __SGCT__SPOUTFLAT__H__
