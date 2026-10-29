/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection/equirectangular.h>

#include <sgct/engine.h>
#include <sgct/log.h>
#include <sgct/profiling.h>
#include <sgct/window.h>
#include <sgct/vulkan/context.h>
#include <sgct/vulkan/cubemaptarget.h>
#include <sgct/vulkan/projectionpass.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <span>
#include <string>
#include <string_view>

namespace {
    // The quad covering the whole viewport (xy is the position, zw the texture
    // coordinates), drawn as a triangle strip
    constexpr std::array<sgct::vec4, 4> FullQuad = {
        sgct::vec4{ -1.f, -1.f, 0.f, 0.f },
        sgct::vec4{ -1.f,  1.f, 0.f, 1.f },
        sgct::vec4{  1.f, -1.f, 1.f, 0.f },
        sgct::vec4{  1.f,  1.f, 1.f, 1.f }
    };

    constexpr std::string_view FragmentShader = R"(
  layout (location = 0) in vec2 in_texCoords;
  layout (location = 0) out vec4 out_diffuse;

  layout (set = 0, binding = 0) uniform samplerCube cubemap;
#if HAS_NORMALS
  layout (set = 0, binding = 1) uniform samplerCube normalmap;
#endif
#if HAS_POSITIONS
  layout (set = 0, binding = 2) uniform samplerCube positionmap;
#endif
#if HAS_DEPTH
  layout (set = 0, binding = 3) uniform samplerCube depthmap;
#endif

  const float PI = 3.141592654;

  // Writes the other attachments of the cube map in the (mirrored) direction
  void writeAttachments(vec3 dir) {
#if HAS_NORMALS
    out_normal = texture(normalmap, dir);
#endif
#if HAS_POSITIONS
    out_position = texture(positionmap, dir);
#endif
#if HAS_DEPTH
    gl_FragDepth = texture(depthmap, dir).r;
#endif
  }


  void main() {
    float phi = PI * (1.0 - in_texCoords.t);
    float theta = 2.0 * PI * (in_texCoords.s - 0.5);
    float x = sin(phi) * sin(theta);
    float y = sin(phi) * cos(theta);
    float z = cos(phi);
    // The cube map is the mirror image of the OpenGL one along the y axis (see
    // vulkan::CubemapTarget)
    vec3 dir = vec3(x, -y, z);
    out_diffuse = texture(cubemap, dir);
    writeAttachments(dir);
  }
)";
} // namespace

namespace sgct {

EquirectangularProjection::EquirectangularProjection(
                                          const config::EquirectangularProjection& config,
                                                                     const Window& parent,
                                                                               User& user)
    : NonLinearProjection(parent)
{
    setUser(user);
    if (config.quality) {
        setCubemapResolution(*config.quality);
    }
}

EquirectangularProjection::~EquirectangularProjection() = default;

void EquirectangularProjection::render(VkCommandBuffer cmd, const BaseViewport&,
                                       FrustumMode frustumMode,
                                       const VkRect2D& area) const
{
    ZoneScoped;

    clearViewport(cmd, area);

    _pass->bind(cmd, FullQuad.data());
    _pass->setTextures(cmd, cubeTextures(frustumMode, TextureView::Cube));
    _pass->drawQuad(cmd);
}

void EquirectangularProjection::update(const vec2&) const {}

void EquirectangularProjection::initViewports() {
    // Radius is needed to calculate the distance to all view planes
    // const float radius = _diameter / 2.f;
    const float radius = 1.f;

    // Setup base viewport that will be rotated to create the other cubemap views
    // +Z face
    const glm::vec4 lowerLeftBase(-radius, -radius, radius, 1.f);
    const glm::vec4 upperLeftBase(-radius, radius, radius, 1.f);
    const glm::vec4 upperRightBase(radius, radius, radius, 1.f);

    const glm::mat4 tiltMat = glm::rotate(
        glm::mat4(1.f),
        glm::radians(90.f),
        glm::vec3(1.f, 0.f, 0.f)
    );

    const glm::mat4 rollRot = glm::rotate(
        tiltMat,
        glm::radians(45.f),
        glm::vec3(0.f, 0.f, 1.f)
    );

    // +X face
    {
        const glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(-90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );

        glm::vec4 upperRight = upperRightBase;
        upperRight.x = radius;

        const glm::vec3 ll = glm::vec3(rotMat * lowerLeftBase);
        const glm::vec3 ul = glm::vec3(rotMat * upperLeftBase);
        const glm::vec3 ur = glm::vec3(rotMat * upperRight);
        _subViewports.right.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // -X face
    {
        const glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );

        glm::vec4 lowerLeft = lowerLeftBase;
        lowerLeft.x = -radius;
        glm::vec4 upperLeft = upperLeftBase;
        upperLeft.x = -radius;

        const glm::vec3 ll = glm::vec3(rotMat * lowerLeft);
        const glm::vec3 ul = glm::vec3(rotMat * upperLeft);
        const glm::vec3 ur = glm::vec3(rotMat * upperRightBase);
        _subViewports.left.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // +Y face
    {
        const glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(-90.f),
            glm::vec3(1.f, 0.f, 0.f)
        );

        glm::vec4 lowerLeft = lowerLeftBase;
        lowerLeft.y = -radius;

        const glm::vec3 ll = glm::vec3(rotMat * lowerLeft);
        const glm::vec3 ul = glm::vec3(rotMat * upperLeftBase);
        const glm::vec3 ur = glm::vec3(rotMat * upperRightBase);
        _subViewports.bottom.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // -Y face
    {
        const glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(90.f),
            glm::vec3(1.f, 0.f, 0.f)
        );

        glm::vec4 upperLeft = upperLeftBase;
        upperLeft.y = radius;
        glm::vec4 upperRight = upperRightBase;
        upperRight.y = radius;

        const glm::vec3 ll = glm::vec3(rotMat * lowerLeftBase);
        const glm::vec3 ul = glm::vec3(rotMat * upperLeft);
        const glm::vec3 ur = glm::vec3(rotMat * upperRight);
        _subViewports.top.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // +Z face
    {
        const glm::vec3 ll = glm::vec3(rollRot * lowerLeftBase);
        const glm::vec3 ul = glm::vec3(rollRot * upperLeftBase);
        const glm::vec3 ur = glm::vec3(rollRot * upperRightBase);
        _subViewports.front.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }
    // -Z face
    {
        const glm::mat4 r = glm::rotate(
            rollRot,
            glm::radians(180.f),
            glm::vec3(0.f, 1.f, 0.f)
        );
        const glm::vec3 ll = glm::vec3(r * lowerLeftBase);
        const glm::vec3 ul = glm::vec3(r * upperLeftBase);
        const glm::vec3 ur = glm::vec3(r * upperRightBase);
        _subViewports.back.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }
}

void EquirectangularProjection::initPipelines(VkRenderPass renderPass) {
    vulkan::ProjectionPass::Settings settings = projectionPassSettings();
    settings.fragmentShader = fragmentShaderHeader() + std::string(FragmentShader);
    settings.pushConstantSize = sizeof(FullQuad);
    _pass = std::make_unique<vulkan::ProjectionPass>(
        vulkan::Context::instance(),
        renderPass,
        settings
    );
}

} // namespace sgct
