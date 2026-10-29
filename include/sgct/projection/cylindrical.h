/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CYLINDRICAL__H__
#define __SGCT__CYLINDRICAL__H__

#include <sgct/sgctexports.h>
#include <sgct/projection/nonlinearprojection.h>

#include <sgct/math.h>
#include <memory>

namespace sgct {

namespace config { struct CylindricalProjection; }
namespace vulkan { class ProjectionPass; }
class BaseViewport;
class User;
class Window;

class SGCT_EXPORT CylindricalProjection final : public NonLinearProjection {
public:
    CylindricalProjection(const config::CylindricalProjection& config,
        const Window& parent, User& user);
    ~CylindricalProjection() final;

    void render(VkCommandBuffer cmd, const BaseViewport& viewport, FrustumMode mode,
        const VkRect2D& area) const override;

    void update(const vec2& size) const override;

    void setRotation(float rotation);
    void setHeightOffset(float heightOffset);
    void setRadius(float radius);

private:
    void initViewports() override;
    void initPipelines(VkRenderPass renderPass) override;

    float _rotation;
    float _heightOffset;
    float _radius;

    std::unique_ptr<vulkan::ProjectionPass> _pass;
};

} // namespace sgct

#endif // __SGCT__CYLINDRICAL__H__
