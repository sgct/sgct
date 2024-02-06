/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection/sphericalmirror.h>

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/fmt.h>
#include <sgct/log.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <sgct/viewport.h>
#include <sgct/window.h>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    constexpr std::string_view SphericalProjectionVert = R"(
  #version 330 core

  layout (location = 0) in vec2 in_position;
  layout (location = 1) in vec2 in_texCoords;
  layout (location = 2) in vec4 in_vertColor;
  out vec2 tr_uv;
  out vec4 tr_color;

  uniform mat4 mvp;

  void main() {
    gl_Position = mvp * vec4(in_position, 0.0, 1.0);
    tr_uv = in_texCoords;
    tr_color = in_vertColor;
  }
)";

    constexpr std::string_view SphericalProjectionFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D tex;

  void main() { out_color = tr_color * texture(tex, tr_uv); }
)";
} // namespace

namespace sgct {

SphericalMirrorProjection::SphericalMirrorProjection(const Window* parent,
                                                     std::string bottomMesh,
                                                     std::string leftMesh,
                                                     std::string rightMesh,
                                                     std::string topMesh)
    : NonLinearProjection(parent)
    , _meshPathBottom(std::move(bottomMesh))
    , _meshPathLeft(std::move(leftMesh))
    , _meshPathRight(std::move(rightMesh))
    , _meshPathTop(std::move(topMesh))
{
    setUseDepthTransformation(false);
}

SphericalMirrorProjection::~SphericalMirrorProjection() {
    _shader.deleteProgram();
    _depthCorrectionShader.deleteProgram();
}

void SphericalMirrorProjection::update(vec2) {}

void SphericalMirrorProjection::render(const Window& window, const BaseViewport& viewport,
                                       Frustum::Mode frustumMode)
{
    ZoneScoped;

    Engine::instance().setupViewport(window, viewport, frustumMode);

    const float aspect = window.aspectRatio() * viewport.size().x / viewport.size().y;
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable: 4127) // warning C4127: conditional expression is constant
#endif // WIN32
    const glm::mat4 mvp = glm::ortho(-aspect, aspect, -1.f, 1.f, -1.f, 1.f);
#ifdef WIN32
#pragma warning(pop)
#endif // WIN32

    glClearColor(_clearColor.x, _clearColor.y, _clearColor.z, _clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _shader.bind();

    glActiveTexture(GL_TEXTURE0);

    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    glUniform1i(_texLoc, 0);
    glUniformMatrix4fv(_matrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindTexture(GL_TEXTURE_2D, _textures.cubeFaceFront);
    _meshBottom.renderWarpMesh();

    glBindTexture(GL_TEXTURE_2D, _textures.cubeFaceLeft);
    _meshLeft.renderWarpMesh();

    glBindTexture(GL_TEXTURE_2D, _textures.cubeFaceRight);
    _meshRight.renderWarpMesh();

    glBindTexture(GL_TEXTURE_2D, _textures.cubeFaceTop);
    _meshTop.renderWarpMesh();

    ShaderProgram::unbind();

    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void SphericalMirrorProjection::renderCubemap(Window& window, Frustum::Mode frustumMode) {
    ZoneScoped;

    auto renderInternal = [this, &window, frustumMode](BaseViewport& bv, unsigned int t) {
        if (!bv.isEnabled()) {
            return;
        }
        _cubeMapFbo->bind();
        if (!_cubeMapFbo->isMultiSampled()) {
            _cubeMapFbo->attachColorTexture(t, GL_COLOR_ATTACHMENT0);
        }

        // Draw Cube Face
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDepthFunc(GL_LESS);

        setupViewport(bv);

        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        RenderData renderData(
            window,
            bv,
            frustumMode,
            ClusterManager::instance().sceneTransform(),
            bv.projection(frustumMode).viewMatrix(),
            bv.projection(frustumMode).projectionMatrix(),
            bv.projection(frustumMode).viewProjectionMatrix() *
                ClusterManager::instance().sceneTransform(),
            _cubemapResolution
        );
        Engine::instance().drawFunction()(renderData);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        if (_cubeMapFbo->isMultiSampled()) {
            // blit MSAA fbo to texture
            _cubeMapFbo->bindBlit();
            _cubeMapFbo->attachColorTexture(t, GL_COLOR_ATTACHMENT0);
            _cubeMapFbo->blit();
        }
    };

    renderInternal(_subViewports.right, _textures.cubeFaceRight);
    renderInternal(_subViewports.left, _textures.cubeFaceLeft);
    renderInternal(_subViewports.bottom, _textures.cubeFaceBottom);
    renderInternal(_subViewports.top, _textures.cubeFaceTop);
    renderInternal(_subViewports.front, _textures.cubeFaceFront);
    renderInternal(_subViewports.back, _textures.cubeFaceBack);
}

void SphericalMirrorProjection::setTilt(float angle) {
    _tilt = angle;
}

void SphericalMirrorProjection::initTextures() {
    auto generate = [this](const BaseViewport& bv, unsigned int& texture) {
        if (!bv.isEnabled()) {
            return;
        }
        generateMap(texture, _texInternalFormat, _texFormat, _texType);
        Log::Debug(fmt::format(
            "{}x{} cube face texture (id: {}) generated",
            _cubemapResolution.x, _cubemapResolution.y, texture
        ));
    };

    generate(_subViewports.right, _textures.cubeFaceRight);
    generate(_subViewports.left, _textures.cubeFaceLeft);
    generate(_subViewports.bottom, _textures.cubeFaceBottom);
    generate(_subViewports.top, _textures.cubeFaceTop);
    generate(_subViewports.front, _textures.cubeFaceFront);
    generate(_subViewports.back, _textures.cubeFaceBack);
}

void SphericalMirrorProjection::initVBO() {
    _meshBottom.loadMesh(_meshPathBottom, _subViewports.bottom);
    _meshLeft.loadMesh(_meshPathLeft, _subViewports.left);
    _meshRight.loadMesh(_meshPathRight, _subViewports.right);
    _meshTop.loadMesh(_meshPathTop, _subViewports.top);
}

void SphericalMirrorProjection::initViewports() {
    // radius is needed to calculate the distance to all view planes
    const float radius = _diameter / 2.f;

    // setup base viewport that will be rotated to create the other cubemap views
    // +Z face
    const glm::vec4 lowerLeftBase(-radius, -radius, radius, 1.f);
    const glm::vec4 upperLeftBase(-radius, radius, radius, 1.f);
    const glm::vec4 upperRightBase(radius, radius, radius, 1.f);

    // tilt
    glm::mat4 tiltMat = glm::rotate(
        glm::mat4(1.f),
        glm::radians(45.f - _tilt),
        glm::vec3(1.f, 0.f, 0.f)
    );

    // Right
    {
        glm::mat4 r = glm::rotate(tiltMat, glm::radians(-90.f), glm::vec3(0.f, 1.f, 0.f));
        glm::vec3 ll = glm::vec3(r * lowerLeftBase);
        glm::vec3 ul = glm::vec3(r * upperLeftBase);
        glm::vec3 ur = glm::vec3(r * upperRightBase);
        _subViewports.right.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // left
    {
        glm::mat4 r = glm::rotate(tiltMat, glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
        glm::vec3 ll = glm::vec3(r * lowerLeftBase);
        glm::vec3 ul = glm::vec3(r * upperLeftBase);
        glm::vec3 ur = glm::vec3(r * upperRightBase);
        _subViewports.left.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // bottom
    _subViewports.bottom.setEnabled(false);

    // top
    {
        glm::mat4 r = glm::rotate(tiltMat, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
        glm::vec3 ll = glm::vec3(r * lowerLeftBase);
        glm::vec3 ul = glm::vec3(r * upperLeftBase);
        glm::vec3 ur = glm::vec3(r * upperRightBase);
        _subViewports.top.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // front
    {
        glm::vec3 ll = glm::vec3(tiltMat * lowerLeftBase);
        glm::vec3 ul = glm::vec3(tiltMat * upperLeftBase);
        glm::vec3 ur = glm::vec3(tiltMat * upperRightBase);
        _subViewports.front.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // back
    _subViewports.back.setEnabled(false);
}

void SphericalMirrorProjection::initShaders() {
    if (_isStereo || _preferedMonoFrustumMode != Frustum::Mode::MonoEye) {
        // if any frustum mode other than Mono (or stereo)
        Log::Warning("Stereo not supported in spherical projection");
    }

    // reload shader program if it exists
    _shader.deleteProgram();

    _shader = ShaderProgram("SphericalMirrorShader");
    _shader.addShaderSource(SphericalProjectionVert, GL_VERTEX_SHADER);
    _shader.addShaderSource(SphericalProjectionFrag, GL_FRAGMENT_SHADER);
    _shader.createAndLinkProgram();
    _shader.bind();

    _texLoc = glGetUniformLocation(_shader.id(), "tex");
    glUniform1i(_texLoc, 0);

    _matrixLoc = glGetUniformLocation(_shader.id(), "mvp");

    ShaderProgram::unbind();
}

} // namespace sgct
