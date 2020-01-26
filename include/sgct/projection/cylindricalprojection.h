/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CYLINDRICALPROJECTION__H__
#define __SGCT__CYLINDRICALPROJECTION__H__

#include <sgct/projection/nonlinearprojection.h>

#include <sgct/callbackdata.h>
#include <sgct/frustum.h>

namespace sgct {

class CylindricalProjection : public NonLinearProjection {
public:
    CylindricalProjection(const Window* parent);

    void render(const Window& window, const BaseViewport& viewport,
        Frustum::Mode) override;

    void renderCubemap(Window& window, Frustum::Mode frustumMode) override;

    void update(glm::vec2 size) override;



    void setRotation(float rotation);

private:
    void initViewports() override;
    void initShaders() override;

    void drawCubeFace(BaseViewport& face, RenderData renderData);
    void blitCubeFace(int face);
    void attachTextures(int face);


    float _rotation = 0.f;

    struct {
        int cubemap = -1;
        int size = -1;
        // int depthCubemap = -1;
        // int normalCubemap = -1;
        // int positionCubemap = -1;
        // int halfFov = -1;
        // int offset = -1;
        // int swapColor = -1;
        // int swapDepth = -1;
        // int swapNear = -1;
        // int swapFar = -1;
    } _shaderLoc;

};


} // namespace sgct

#endif // __SGCT__CYLINDRICALPROJECTION__H__
