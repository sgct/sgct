/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/sphericalmirrorprojection.h>

#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/viewport.h>
#include <sgct/window.h>
#include <sgct/helpers/stringfunctions.h>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    constexpr const char* SphericalProjectionVert = R"(
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

    constexpr const char* SphericalProjectionFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D tex;

  void main() {
    out_color = tr_color * texture(tex, tr_uv);
  }
)";
} // namespace

namespace sgct::core {

SphericalMirrorProjection::SphericalMirrorProjection(std::string bottomMesh,
                                                     std::string leftMesh,
                                                     std::string rightMesh,
                                                     std::string topMesh)
    : _meshPaths{
        std::move(bottomMesh),
        std::move(leftMesh),
        std::move(rightMesh),
        std::move(topMesh)
    }
{
    setUseDepthTransformation(false);
}

void SphericalMirrorProjection::update(glm::vec2) {}

void SphericalMirrorProjection::render() {
    Engine::instance().enterCurrentViewport();

    Window& win = Engine::instance().getCurrentWindow();
    BaseViewport* vpPtr = win.getCurrentViewport();

    float aspect = win.getAspectRatio() * (vpPtr->getSize().x / vpPtr->getSize().y);
    glm::mat4 mvp = glm::ortho(-aspect, aspect, -1.f, 1.f, -1.f, 1.f);

    glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _shader.bind();

    glActiveTexture(GL_TEXTURE0);

    glDisable(GL_CULL_FACE);
    const bool hasAlpha = Engine::instance().getCurrentWindow().hasAlpha();
    if (hasAlpha) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else {
        glDisable(GL_BLEND);
    }
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    glUniform1i(_texLoc, 0);
    glUniformMatrix4fv(_matrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindTexture(GL_TEXTURE_2D, _textures.cubeFaceFront);
    _meshes.bottom.renderWarpMesh();

    glBindTexture(GL_TEXTURE_2D, _textures.cubeFaceLeft);
    _meshes.left.renderWarpMesh();

    glBindTexture(GL_TEXTURE_2D, _textures.cubeFaceRight);
    _meshes.right.renderWarpMesh();

    glBindTexture(GL_TEXTURE_2D, _textures.cubeFaceTop);
    _meshes.top.renderWarpMesh();

    ShaderProgram::unbind();

    glDisable(GL_DEPTH_TEST);

    if (hasAlpha) {
        glDisable(GL_BLEND);
    }

    glDepthFunc(GL_LESS);
}

void SphericalMirrorProjection::renderCubemap() {
    auto renderInternal = [this](BaseViewport& bv, unsigned int texture) {
        if (!bv.isEnabled()) {
            return;
        }
        _cubeMapFbo->bind();
        if (!_cubeMapFbo->isMultiSampled()) {
            _cubeMapFbo->attachColorTexture(texture);
        }

        Engine::instance().getCurrentWindow().setCurrentViewport(&bv);

        // Draw Cube Face
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDepthFunc(GL_LESS);

        setupViewport(bv);

        const glm::vec4 color = Engine::instance().getClearColor();
        const float a = Engine::instance().getCurrentWindow().hasAlpha() ? 0.f : color.a;
        glClearColor(color.r, color.g, color.b, a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Engine::instance().getDrawFunction()();

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        if (_cubeMapFbo->isMultiSampled()) {
            // blit MSAA fbo to texture
            _cubeMapFbo->bindBlit();
            _cubeMapFbo->attachColorTexture(texture);
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

void core::SphericalMirrorProjection::initTextures() {
    auto generate = [this](const BaseViewport& bv, unsigned int& texture) {
        if (!bv.isEnabled()) {
            return;
        }
        generateMap(texture, _texInternalFormat);
        MessageHandler::printDebug(
            "%dx%d cube face texture (id: %d) generated",
            _cubemapResolution, _cubemapResolution, texture
        );
    };

    generate(_subViewports.right, _textures.cubeFaceRight);
    generate(_subViewports.left, _textures.cubeFaceLeft);
    generate(_subViewports.bottom, _textures.cubeFaceBottom);
    generate(_subViewports.top, _textures.cubeFaceTop);
    generate(_subViewports.front, _textures.cubeFaceFront);
    generate(_subViewports.back, _textures.cubeFaceBack);
}

void SphericalMirrorProjection::initVBO() {
    Viewport* vp = dynamic_cast<Viewport*>(
        Engine::instance().getCurrentWindow().getCurrentViewport()
    );
    if (vp) {
        _meshes.bottom.loadMesh(_meshPaths.bottom, *vp);
        _meshes.left.loadMesh(_meshPaths.left, *vp);
        _meshes.right.loadMesh(_meshPaths.right, *vp);
        _meshes.top.loadMesh(_meshPaths.top, *vp);
    }
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
        _subViewports.right.getProjectionPlane().setCoordinates(
            glm::vec3(r * lowerLeftBase),
            glm::vec3(r * upperLeftBase),
            glm::vec3(r * upperRightBase)
        );
    }

    // left
    {
        glm::mat4 r = glm::rotate(tiltMat, glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
        _subViewports.left.getProjectionPlane().setCoordinates(
            glm::vec3(r * lowerLeftBase),
            glm::vec3(r * upperLeftBase),
            glm::vec3(r * upperRightBase)
        );
    }

    // bottom
    _subViewports.bottom.setEnabled(false);

    // top
    {
        glm::mat4 r = glm::rotate(tiltMat, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
        _subViewports.top.getProjectionPlane().setCoordinates(
            glm::vec3(r * lowerLeftBase),
            glm::vec3(r * upperLeftBase),
            glm::vec3(r * upperRightBase)
        );
    }

    // front
    {
        _subViewports.front.getProjectionPlane().setCoordinates(
            glm::vec3(tiltMat * lowerLeftBase),
            glm::vec3(tiltMat * upperLeftBase),
            glm::vec3(tiltMat * upperRightBase)
        );
    }

    // back
    _subViewports.back.setEnabled(false);
}

void SphericalMirrorProjection::initShaders() {
    if (_isStereo || _preferedMonoFrustumMode != Frustum::Mode::MonoEye) {
        // if any frustum mode other than Mono (or stereo)
        MessageHandler::printWarning("Stereo not supported in spherical projection");
    }

    // reload shader program if it exists
    if (_shader.isLinked()) {
        _shader.deleteProgram();
    }

    _shader = ShaderProgram("SphericalMirrorShader");
    _shader.addShaderSource(SphericalProjectionVert, SphericalProjectionFrag);
    _shader.createAndLinkProgram();
    _shader.bind();

    _texLoc = glGetUniformLocation(_shader.getId(), "tex");
    glUniform1i(_texLoc, 0);

    _matrixLoc = glGetUniformLocation(_shader.getId(), "mvp");

    ShaderProgram::unbind();
}

} // namespace sgct::core
