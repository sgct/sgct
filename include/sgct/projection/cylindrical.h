/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CYLINDRICAL__H__
#define __SGCT__CYLINDRICAL__H__

#include <sgct/sgctexports.h>
#include <sgct/projection/nonlinearprojection.h>

#include <sgct/definitions.h>
#include <sgct/math.h>
#include <sgct/shaderprogram.h>

namespace sgct {

namespace config { struct CylindricalProjection; }

class BaseViewport;
class Window;
class User;

class SGCT_EXPORT CylindricalProjection final : public NonLinearProjection {
public:
    CylindricalProjection(const config::CylindricalProjection& config,
        const Window& parent, User& user);
    ~CylindricalProjection() final;

    void render(const BaseViewport& viewport, FrustumMode mode) const override;

    void renderCubemap(FrustumMode frustumMode) const override;

    void update(const vec2& size) const override;

    void setRotation(float rotation);
    void setHeightOffset(float heightOffset);
    void setRadius(float radius);

private:
    void initVBO() override;
    void initViewports() override;
    void initShaders() override;

    float _rotation;
    float _heightOffset;
    float _radius;

    struct {
        ShaderProgram program;
        int cubemap = -1;
        int rotation = -1;
        int heightOffset = -1;
    } _shader;

    unsigned int _vao = 0;
    unsigned int _vbo = 0;
};

} // namespace sgct

#endif // __SGCT__CYLINDRICAL__H__
