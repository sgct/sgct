/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__EQUIRECTANGULAR__H__
#define __SGCT__EQUIRECTANGULAR__H__

#include <sgct/sgctexports.h>
#include <sgct/projection/nonlinearprojection.h>

#include <sgct/definitions.h>
#include <sgct/math.h>
#include <sgct/shaderprogram.h>

namespace sgct {

namespace config { struct EquirectangularProjection; }

class BaseViewport;
class Window;
class User;

class SGCT_EXPORT EquirectangularProjection final : public NonLinearProjection {
public:
    EquirectangularProjection(const config::EquirectangularProjection& config,
        const Window& parent, User& user);
    ~EquirectangularProjection() final;

    void render(const BaseViewport& viewport, FrustumMode mode) const override;

    void renderCubemap(FrustumMode frustumMode) const override;

    void update(const vec2& size) const override;

private:
    void initVBO() override;
    void initViewports() override;
    void initShaders() override;

    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    ShaderProgram _shader;
};

} // namespace sgct

#endif // __SGCT__EQUIRECTANGULAR__H__
