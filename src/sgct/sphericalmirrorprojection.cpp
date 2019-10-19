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
#include <sgct/window.h>
#include <sgct/viewport.h>
#include <sgct/helpers/stringfunctions.h>
#include <sgct/shaders/internalsphericalprojectionshaders.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//#define DebugCubemap

namespace sgct::core {

void SphericalMirrorProjection::update(glm::vec2) {}

void SphericalMirrorProjection::render() {
    Engine::instance()->enterCurrentViewport();

    Window& winPtr = Engine::instance()->getCurrentWindow();
    BaseViewport* vpPtr = winPtr.getCurrentViewport();

    float aspect = winPtr.getAspectRatio() * (vpPtr->getSize().x / vpPtr->getSize().y);
    glm::mat4 MVP = glm::ortho(-aspect, aspect, -1.f, 1.f, -1.f, 1.f);

    glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _shader.bind();

    // if for some reson the active texture has been reset
    glActiveTexture(GL_TEXTURE0);

    glDisable(GL_CULL_FACE);
    const bool alpha = Engine::instance()->getCurrentWindow().getAlpha();
    if (alpha) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else {
        glDisable(GL_BLEND);
    }
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    glUniform1i(_texLoc, 0);
    glUniformMatrix4fv(_matrixLoc, 1, GL_FALSE, glm::value_ptr(MVP));

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

    if (alpha) {
        glDisable(GL_BLEND);
    }

    // restore depth func
    glDepthFunc(GL_LESS);
}

void SphericalMirrorProjection::renderCubemap(size_t* subViewPortIndex) {
    auto renderInternal =
        [this, subViewPortIndex](BaseViewport& bv, unsigned int& texture, int idx)
    {
        *subViewPortIndex = static_cast<size_t>(idx);
        if (!bv.isEnabled()) {
            return;
        }
        _cubeMapFbo->bind();
        if (!_cubeMapFbo->isMultiSampled()) {
            attachTextures(texture);
        }

        Engine::instance()->getCurrentWindow().setCurrentViewport(&bv);
        drawCubeFace(idx);

        // blit MSAA fbo to texture
        if (_cubeMapFbo->isMultiSampled()) {
            blitCubeFace(texture);
        }
    };

    renderInternal(_subViewports.right, _textures.cubeFaceRight, 0);
    renderInternal(_subViewports.left, _textures.cubeFaceLeft, 1);
    renderInternal(_subViewports.bottom, _textures.cubeFaceBottom, 2);
    renderInternal(_subViewports.top, _textures.cubeFaceTop, 3);
    renderInternal(_subViewports.front, _textures.cubeFaceFront, 4);
    renderInternal(_subViewports.back, _textures.cubeFaceBack, 5);
}

void SphericalMirrorProjection::setTilt(float angle) {
    _tilt = angle;
}

void SphericalMirrorProjection::setMeshPaths(std::string bottom, std::string left,
                                             std::string right, std::string top)
{
    _meshPaths.bottom = std::move(bottom);
    _meshPaths.left = std::move(left);
    _meshPaths.right = std::move(right);
    _meshPaths.top = std::move(top);
}

void core::SphericalMirrorProjection::initTextures() {
    auto generate = [this](const BaseViewport& bv, unsigned int& texture) {
        if (!bv.isEnabled()) {
            return;
        }
        generateMap(texture, _texInternalFormat);
        if (Engine::checkForOGLErrors("SphericalMirrorProjection::initTextures")) {
            MessageHandler::printDebug(
                "NonLinearProjection: %dx%d cube face texture (id: %d) generated",
                _cubemapResolution, _cubemapResolution, texture
            );
        }
        else {
            MessageHandler::printError(
                "NonLinearProjection: Error occured while generating %dx%d cube face "
                "texture (id: %d)", _cubemapResolution, _cubemapResolution, texture
            );
        }
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
        Engine::instance()->getCurrentWindow().getCurrentViewport()
    );
    if (vp) {
        _meshes.bottom.readAndGenerateMesh(_meshPaths.bottom, *vp);
        _meshes.left.readAndGenerateMesh(_meshPaths.left, *vp);
        _meshes.right.readAndGenerateMesh(_meshPaths.right, *vp);
        _meshes.top.readAndGenerateMesh(_meshPaths.top, *vp);
    }
}

void SphericalMirrorProjection::initViewports() {
    // radius is needed to calculate the distance to all view planes
    float radius = _diameter / 2.f;

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
        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 rotMat = glm::rotate(
            tiltMat,
            glm::radians(-90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );

        _subViewports.right.getProjectionPlane().setCoordinateLowerLeft(
            glm::vec3(rotMat * lowerLeft)
        );
        _subViewports.right.getProjectionPlane().setCoordinateUpperLeft(
            glm::vec3(rotMat * upperLeft)
        );
        _subViewports.right.getProjectionPlane().setCoordinateUpperRight(
            glm::vec3(rotMat * upperRight)
        );
    }

    // left
    {
        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 rotMat = glm::rotate(
            tiltMat,
            glm::radians(90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );

        _subViewports.left.getProjectionPlane().setCoordinateLowerLeft(
            glm::vec3(rotMat * lowerLeft)
        );
        _subViewports.left.getProjectionPlane().setCoordinateUpperLeft(
            glm::vec3(rotMat * upperLeft)
        );
        _subViewports.left.getProjectionPlane().setCoordinateUpperRight(
            glm::vec3(rotMat * upperRight)
        );
    }

    // bottom
    {
        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        _subViewports.bottom.setEnabled(false);
        glm::mat4 rotMat = glm::rotate(
            tiltMat,
            glm::radians(-90.f),
            glm::vec3(1.f, 0.f, 0.f)
        );

        _subViewports.bottom.getProjectionPlane().setCoordinateLowerLeft(
            glm::vec3(rotMat * lowerLeft)
        );
        _subViewports.bottom.getProjectionPlane().setCoordinateUpperLeft(
            glm::vec3(rotMat * upperLeft)
        );
        _subViewports.bottom.getProjectionPlane().setCoordinateUpperRight(
            glm::vec3(rotMat * upperRight)
        );
    }

    // top
    {
        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 rotMat = glm::rotate(
            tiltMat,
            glm::radians(90.f),
            glm::vec3(1.f, 0.f, 0.f)
        );

        _subViewports.top.getProjectionPlane().setCoordinateLowerLeft(
            glm::vec3(rotMat * lowerLeft)
        );
        _subViewports.top.getProjectionPlane().setCoordinateUpperLeft(
            glm::vec3(rotMat * upperLeft)
        );
        _subViewports.top.getProjectionPlane().setCoordinateUpperRight(
            glm::vec3(rotMat * upperRight)
        );
    }

    // front
    {
        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        _subViewports.front.getProjectionPlane().setCoordinateLowerLeft(
            glm::vec3(tiltMat * lowerLeft)
        );
        _subViewports.front.getProjectionPlane().setCoordinateUpperLeft(
            glm::vec3(tiltMat * upperLeft)
        );
        _subViewports.front.getProjectionPlane().setCoordinateUpperRight(
            glm::vec3(tiltMat * upperRight)
        );
    }

    // back
    {
        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;
        
        _subViewports.back.setEnabled(false);

        glm::mat4 rotMat = glm::rotate(
            tiltMat,
            glm::radians(180.f),
            glm::vec3(0.f, 1.f, 0.f)
        );

        _subViewports.back.getProjectionPlane().setCoordinateLowerLeft(
            glm::vec3(rotMat * lowerLeft)
        );
        _subViewports.back.getProjectionPlane().setCoordinateUpperLeft(
            glm::vec3(rotMat * upperLeft)
        );
        _subViewports.back.getProjectionPlane().setCoordinateUpperRight(
            glm::vec3(rotMat * upperRight)
        );
    }
}

void SphericalMirrorProjection::initShaders() {
    if (_stereo || _preferedMonoFrustumMode != Frustum::Mode::MonoEye) {
        // if any frustum mode other than Mono (or stereo)
        MessageHandler::printWarning(
            "Stereo rendering not supported in spherical projection"
        );
    }

    //reload shader program if it exists
    if (_shader.isLinked()) {
        _shader.deleteProgram();
    }

    _shader.addShaderSource(
        core::shaders::SphericalProjectionVert,
        core::shaders::SphericalProjectionFrag
    );
    _shader.setName("SphericalMirrorShader");
    _shader.createAndLinkProgram();
    _shader.bind();

    _texLoc = _shader.getUniformLocation("Tex");
    glUniform1i(_texLoc, 0);

    _matrixLoc = _shader.getUniformLocation("MVP");

    ShaderProgram::unbind();
}

void SphericalMirrorProjection::drawCubeFace(size_t face) {
    BaseViewport& vp = [this](size_t face) -> BaseViewport& {
        switch (face) {
            default:
            case 0: return _subViewports.right;
            case 1: return _subViewports.left;
            case 2: return _subViewports.bottom;
            case 3: return _subViewports.top;
            case 4: return _subViewports.front;
            case 5: return _subViewports.back;
        }
    }(face);

    glLineWidth(1.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDepthFunc(GL_LESS);

    setupViewport(vp);

#ifdef DebugCubemap
    float color[4];
    switch (face) {
        case 0:
            color[0] = 0.5f;
            color[1] = 0.f;
            color[2] = 0.f;
            color[3] = 1.f;
            break;
        case 1:
            color[0] = 0.5f;
            color[1] = 0.5f;
            color[2] = 0.f;
            color[3] = 1.f;
            break;
        case 2:
            color[0] = 0.f;
            color[1] = 0.5f;
            color[2] = 0.f;
            color[3] = 1.f;
            break;
        case 3:
            color[0] = 0.f;
            color[1] = 0.5f;
            color[2] = 0.5f;
            color[3] = 1.f;
            break;
        case 4:
            color[0] = 0.f;
            color[1] = 0.f;
            color[2] = 0.5f;
            color[3] = 1.f;
            break;
        case 5:
            color[0] = 0.5f;
            color[1] = 0.f;
            color[2] = 0.5f;
            color[3] = 1.f;
            break;
    }
    glClearColor(color[0], color[1], color[2], color[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
    Engine::clearBuffer();
#endif

    Engine::instance()->_drawFn();

    // restore polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void SphericalMirrorProjection::blitCubeFace(unsigned int texture) {
    // copy AA-buffer to "regular"/non-AA buffer
    // bind separate read and draw buffers to prepare blit operation
    _cubeMapFbo->bindBlit();
    attachTextures(texture);
    _cubeMapFbo->blit();
}

void SphericalMirrorProjection::attachTextures(unsigned int texture) {
    _cubeMapFbo->attachColorTexture(texture);
}

} // namespace sgct::core
