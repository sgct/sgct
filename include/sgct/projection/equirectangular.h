/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__EQUIRECTANGULAR__H__
#define __SGCT__EQUIRECTANGULAR__H__

#include <sgct/projection/nonlinearprojection.h>

#include <sgct/callbackdata.h>
#include <sgct/frustum.h>

namespace sgct {

class EquirectangularProjection : public NonLinearProjection {
public:
    EquirectangularProjection(const Window* parent);
    ~EquirectangularProjection();

    void render(const Window& window, const BaseViewport& viewport,
        Frustum::Mode) override;

    void renderCubemap(Window& window, Frustum::Mode frustumMode) override;

    void update(glm::vec2 size) override;

private:
    void initVBO() override;
    void initViewports() override;
    void initShaders() override;

    struct {
        int cubemap = -1;
    } _shaderLoc;
    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    ShaderProgram _shader;
    ShaderProgram _depthCorrectionShader;
};

} // namespace sgct

#endif // __SGCT__EQUIRECTANGULAR__H__
