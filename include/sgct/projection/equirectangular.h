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

    void render(const Window& window, const BaseViewport& viewport,
        Frustum::Mode) override;

    void renderCubemap(Window& window, Frustum::Mode frustumMode) override;

    void update(glm::vec2 size) override;

private:
    void initViewports() override;
    void initShaders() override;

    void drawCubeFace(BaseViewport& face, RenderData renderData);
    void blitCubeFace(int face);
    void attachTextures(int face);

    struct {
        int cubemap = -1;
    } _shaderLoc;
};

} // namespace sgct

#endif // __SGCT__EQUIRECTANGULAR__H__
