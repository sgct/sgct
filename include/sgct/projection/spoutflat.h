/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__SPOUTFLAT__H__
#define __SGCT__SPOUTFLAT__H__

#include <sgct/projection/nonlinearprojection.h>
#include <sgct/projection/projectionplane.h>
#include <sgct/callbackdata.h>

namespace sgct {

/// Base class for non linear projections
class SpoutFlatProjection : public NonLinearProjection {
public:
    SpoutFlatProjection(const Window* parent);

    virtual ~SpoutFlatProjection();

    virtual void render(const Window& window, const BaseViewport& viewport,
        Frustum::Mode frustumMode);
    virtual void renderCubemap(Window& window, Frustum::Mode frustumMode);
    virtual void update(vec2 size);

    void setSpoutMappingName(std::string name); 
    void setResolutionWidth(int resolutionX);
    void setResolutionHeight(int resolutionY);
    void setSpoutFov(float up, float down, float left, float right, quat orientation,
        float distance);
    void setSpoutOffset(vec3 offset);
    void setSpoutDrawMain(bool drawMain);

protected:
    virtual void initTextures();
    virtual void initFBO();
    virtual void initVBO();
    virtual void initViewports();
    virtual void initShaders();

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
    } _textures;

    std::string _mappingName = "SPOUT_OS_MAPPING";
    void* _mappingHandle = nullptr;
    int _resolutionX = 1024;
    int _resolutionY = 768;
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

#endif // __SGCT__NONLINEARPROJECTION__H__
