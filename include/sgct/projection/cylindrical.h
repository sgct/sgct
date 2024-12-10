/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CYLINDRICAL__H__
#define __SGCT__CYLINDRICAL__H__

#include <sgct/sgctexports.h>
#include <sgct/projection/nonlinearprojection.h>

#include <sgct/callbackdata.h>
#include <sgct/frustum.h>

namespace sgct {

class SGCT_EXPORT CylindricalProjection final : public NonLinearProjection {
public:
    CylindricalProjection(const Window* parent);
    ~CylindricalProjection() final;

    void render(const Window& window, const BaseViewport& viewport,
        Frustum::Mode) override;

    void renderCubemap(Window& window, Frustum::Mode frustumMode) override;

    void update(vec2 size) override;

    void setRotation(float rotation);
    void setHeightOffset(float heightOffset);
    void setRadius(float radius);

private:
    void initVBO() override;
    void initViewports() override;
    void initShaders() override;

    float _rotation = 0.f;
    float _heightOffset = 0.f;
    float _radius = 5.f;

    struct {
        int cubemap = -1;
        int rotation = -1;
        int heightOffset = -1;
    } _shaderLoc;
    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    ShaderProgram _shader;
};

} // namespace sgct

#endif // __SGCT__CYLINDRICAL__H__
