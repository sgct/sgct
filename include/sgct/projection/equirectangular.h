/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__EQUIRECTANGULAR__H__
#define __SGCT__EQUIRECTANGULAR__H__

#include <sgct/sgctexports.h>
#include <sgct/projection/nonlinearprojection.h>

#include <sgct/math.h>
#include <memory>

namespace sgct {

namespace config { struct EquirectangularProjection; }
namespace vulkan { class ProjectionPass; }
class BaseViewport;
class User;
class Window;

class SGCT_EXPORT EquirectangularProjection final : public NonLinearProjection {
public:
    EquirectangularProjection(const config::EquirectangularProjection& config,
        const Window& parent, User& user);
    ~EquirectangularProjection() final;

    void render(VkCommandBuffer cmd, const BaseViewport& viewport, FrustumMode mode,
        const VkRect2D& area) const override;

    void update(const vec2& size) const override;

private:
    void initViewports() override;
    void initPipelines(VkRenderPass renderPass) override;

    std::unique_ptr<vulkan::ProjectionPass> _pass;
};

} // namespace sgct

#endif // __SGCT__EQUIRECTANGULAR__H__
