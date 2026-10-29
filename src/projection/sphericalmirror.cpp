/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection/sphericalmirror.h>

#include <sgct/engine.h>
#include <sgct/format.h>
#include <sgct/log.h>
#include <sgct/profiling.h>
#include <sgct/viewport.h>
#include <sgct/window.h>
#include <sgct/vulkan/context.h>
#include <sgct/vulkan/cubemaptarget.h>
#include <sgct/vulkan/projectionpass.h>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <span>
#include <string>
#include <string_view>

namespace {
    constexpr const char* SphericalProjectionVert = R"(
  #version 460

  layout (location = 0) in vec2 in_position;
  layout (location = 1) in vec2 in_texCoords;
  layout (location = 2) in vec4 in_vertColor;

  layout (location = 0) out vec2 out_texCoords;
  layout (location = 1) out vec4 out_color;

  layout (push_constant) uniform Constants {
    mat4 mvp;
  } pc;

  void main() {
    gl_Position = pc.mvp * vec4(in_position, 0.0, 1.0);
    out_texCoords = in_texCoords;
    out_color = in_vertColor;
  }
)";

    // The faces are stored upside down compared to OpenGL (see vulkan::CubemapTarget)
    constexpr std::string_view SphericalProjectionFrag = R"(
  layout (location = 0) in vec2 in_texCoords;
  layout (location = 1) in vec4 in_color;

  layout (location = 0) out vec4 out_color;

  layout (set = 0, binding = 0) uniform sampler2D tex;
#if HAS_NORMALS
  layout (set = 0, binding = 1) uniform sampler2D normals;
#endif
#if HAS_POSITIONS
  layout (set = 0, binding = 2) uniform sampler2D positions;
#endif
#if HAS_DEPTH
  layout (set = 0, binding = 3) uniform sampler2D depths;
#endif

  void main() {
    vec2 texCoords = vec2(in_texCoords.x, 1.0 - in_texCoords.y);
    out_color = in_color * texture(tex, texCoords);
#if HAS_NORMALS
    out_normal = texture(normals, texCoords);
#endif
#if HAS_POSITIONS
    out_position = texture(positions, texCoords);
#endif
#if HAS_DEPTH
    gl_FragDepth = texture(depths, texCoords).r;
#endif
  }
)";
} // namespace

namespace sgct {

SphericalMirrorProjection::SphericalMirrorProjection(
                                          const config::SphericalMirrorProjection& config,
                                                                     const Window& parent,
                                                                               User& user)
    : NonLinearProjection(parent)
    , _tilt(config.tilt.value_or(0.f))
    , _meshPathBottom(config.mesh.bottom)
    , _meshPathLeft(config.mesh.left)
    , _meshPathRight(config.mesh.right)
    , _meshPathTop(config.mesh.top)
{
    setUser(user);
    if (config.quality) {
        setCubemapResolution(*config.quality);
    }
    _clearColor = config.background.value_or(_clearColor);
}

SphericalMirrorProjection::~SphericalMirrorProjection() = default;

void SphericalMirrorProjection::update(const vec2&) const {}

void SphericalMirrorProjection::render(VkCommandBuffer cmd, const BaseViewport& viewport,
                                       FrustumMode frustumMode,
                                       const VkRect2D& area) const
{
    ZoneScoped;

    clearViewport(cmd, area);

    const float aspect =
        viewport.window().aspectRatio() * viewport.size().x / viewport.size().y;
    const glm::mat4 mvp = glm::ortho(-aspect, aspect, -1.f, 1.f, -1.f, 1.f);
    _pass->bind(cmd, glm::value_ptr(mvp));

    auto renderMesh = [this, cmd, frustumMode](const CorrectionMesh& mesh, int face) {
        _pass->setTextures(cmd, faceTextures(frustumMode, face));
        mesh.renderWarpMesh(cmd);
    };

    // The faces in the order of the cube map: right, left, bottom, top, front, back
    renderMesh(_meshBottom, 4);
    renderMesh(_meshLeft, 1);
    renderMesh(_meshRight, 0);
    renderMesh(_meshTop, 3);
}

void SphericalMirrorProjection::setTilt(float angle) {
    _tilt = angle;
}

void SphericalMirrorProjection::initViewports() {
    // Radius is needed to calculate the distance to all view planes
    const float radius = _diameter / 2.f;

    // Setup base viewport that will be rotated to create the other cubemap views
    // +Z face
    const glm::vec4 lowerLeftBase(-radius, -radius, radius, 1.f);
    const glm::vec4 upperLeftBase(-radius, radius, radius, 1.f);
    const glm::vec4 upperRightBase(radius, radius, radius, 1.f);

    // Tilt
    const glm::mat4 tiltMat = glm::rotate(
        glm::mat4(1.f),
        glm::radians(45.f - _tilt),
        glm::vec3(1.f, 0.f, 0.f)
    );

    // Right
    {
        const glm::mat4 r = glm::rotate(
            tiltMat,
            glm::radians(-90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );
        const glm::vec3 ll = glm::vec3(r * lowerLeftBase);
        const glm::vec3 ul = glm::vec3(r * upperLeftBase);
        const glm::vec3 ur = glm::vec3(r * upperRightBase);
        _subViewports.right.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // Left
    {
        const glm::mat4 r = glm::rotate(
            tiltMat,
            glm::radians(90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );
        const glm::vec3 ll = glm::vec3(r * lowerLeftBase);
        const glm::vec3 ul = glm::vec3(r * upperLeftBase);
        const glm::vec3 ur = glm::vec3(r * upperRightBase);
        _subViewports.left.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // Bottom
    _subViewports.bottom.setEnabled(false);

    // Top
    {
        const glm::mat4 r = glm::rotate(
            tiltMat,
            glm::radians(90.f),
            glm::vec3(1.f, 0.f, 0.f)
        );
        const glm::vec3 ll = glm::vec3(r * lowerLeftBase);
        const glm::vec3 ul = glm::vec3(r * upperLeftBase);
        const glm::vec3 ur = glm::vec3(r * upperRightBase);
        _subViewports.top.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // Front
    {
        const glm::vec3 ll = glm::vec3(tiltMat * lowerLeftBase);
        const glm::vec3 ul = glm::vec3(tiltMat * upperLeftBase);
        const glm::vec3 ur = glm::vec3(tiltMat * upperRightBase);
        _subViewports.front.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // Back
    _subViewports.back.setEnabled(false);
}

void SphericalMirrorProjection::initPipelines(VkRenderPass renderPass) {
    if (_isStereo || _preferedMonoFrustumMode != FrustumMode::Mono) {
        // If any frustum mode other than Mono (or stereo)
        Log::Warning("Stereo not supported in spherical projection");
    }

    _meshBottom.loadMesh(_meshPathBottom, _subViewports.bottom);
    _meshLeft.loadMesh(_meshPathLeft, _subViewports.left);
    _meshRight.loadMesh(_meshPathRight, _subViewports.right);
    _meshTop.loadMesh(_meshPathTop, _subViewports.top);

    vulkan::ProjectionPass::Settings settings = projectionPassSettings();
    settings.vertexShader = SphericalProjectionVert;
    settings.fragmentShader =
        fragmentShaderHeader() + std::string(SphericalProjectionFrag);
    settings.pushConstantSize = sizeof(glm::mat4);
    settings.useMeshVertices = true;
    _pass = std::make_unique<vulkan::ProjectionPass>(
        vulkan::Context::instance(),
        renderPass,
        settings
    );
}

} // namespace sgct
