/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/fisheyeprojection.h>

#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/settings.h>
#include <sgct/user.h>
#include <sgct/window.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/helpers/stringfunctions.h>
#include <sgct/shaders/internalfisheyeshaders.h>
#include <sgct/shaders/internalfisheyeshaders_cubic.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>

//#define DebugCubemap

namespace sgct::core {

void FisheyeProjection::update(glm::vec2 size) {
    float cropAspect =
        ((1.f - 2.f * _cropFactor.bottom) + (1.f - 2.f * _cropFactor.top)) /
        ((1.f - 2.f * _cropFactor.left) + (1.f - 2.f * _cropFactor.right));

    float x = 1.f;
    float y = 1.f;

    float frameBufferAspect = _ignoreAspectRatio ? 1.f : (size.x / size.y);

    if (Settings::instance()->getTryMaintainAspectRatio()) {
        float aspect = frameBufferAspect * cropAspect;
        if (aspect >= 1.f) {
            x = 1.f / aspect;
        }
        else {
            y = aspect;
        }
    }

    _vertices[0] = _cropFactor.left;
    _vertices[1] = _cropFactor.bottom;
    _vertices[2] = -x;
    _vertices[3] = -y;
    _vertices[4] = -1.f;

    _vertices[5] = _cropFactor.left;
    _vertices[6] = 1.f - _cropFactor.top;
    _vertices[7] = -x;
    _vertices[8] = y;
    _vertices[9] = -1.f;

    _vertices[10] = 1.f - _cropFactor.right;
    _vertices[11] = _cropFactor.bottom;
    _vertices[12] = x;
    _vertices[13] = -y;
    _vertices[14] = -1.f;

    _vertices[15] = 1.f - _cropFactor.right;
    _vertices[16] = 1.f - _cropFactor.top;
    _vertices[17] = x;
    _vertices[18] = y;
    _vertices[19] = -1.f;

    // update VBO
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    GLvoid* PositionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    memcpy(PositionBuffer, _vertices.data(), 20 * sizeof(float));
    glUnmapBuffer(GL_ARRAY_BUFFER);

    glBindVertexArray(0);
}

void FisheyeProjection::render() {
    glEnable(GL_SCISSOR_TEST);
    Engine::instance()->enterCurrentViewport();
    glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    _shader.bind();

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // if for some reson the active texture has been reset
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapColor);

    if (Settings::instance()->useDepthTexture()) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapDepth);
        glUniform1i(_shaderLoc.depthCubemapLoc, 1);
    }

    if (Settings::instance()->useNormalTexture()) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapNormals);
        glUniform1i(_shaderLoc.normalCubemapLoc, 2);
    }

    if (Settings::instance()->usePositionTexture()) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapPositions);
        glUniform1i(_shaderLoc.positionCubemapLoc, 3);
    }

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

    glUniform1i(_shaderLoc.cubemapLoc, 0);
    glUniform1f(_shaderLoc.halfFovLoc, glm::radians<float>(_fov / 2.f));

    if (_offAxis) {
        glUniform3f(_shaderLoc.offsetLoc, _totalOffset.x, _totalOffset.y, _totalOffset.z);
    }

    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    ShaderProgram::unbind();

    glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glDisable(GL_DEPTH_TEST);

    if (alpha) {
        glDisable(GL_BLEND);
    }

    // restore depth func
    glDepthFunc(GL_LESS);
}

void FisheyeProjection::renderCubemap(std::size_t* subViewPortIndex) {
    const Engine& eng = *Engine::instance();
    switch (Engine::instance()->getCurrentFrustumMode()) {
        default:
            break;
        case Frustum::Mode::StereoLeftEye:
            setOffset(glm::vec3(
                -eng.getDefaultUser().getEyeSeparation() / _diameter,
                0.f,
                0.f
            ));
            break;
        case Frustum::Mode::StereoRightEye:
            setOffset(glm::vec3(
                eng.getDefaultUser().getEyeSeparation() / _diameter,
                0.f,
                0.f
            ));
            break;
    }

    auto internalRender = [this, subViewPortIndex](BaseViewport& vp, int idx) {
        *subViewPortIndex = idx;

        if (!vp.isEnabled()) {
            return;
        }

        _cubeMapFbo->bind();
        if (!_cubeMapFbo->isMultiSampled()) {
            attachTextures(idx);
        }

        Engine::instance()->getCurrentWindow().setCurrentViewport(&vp);
        drawCubeFace(vp);

        // blit MSAA fbo to texture
        if (_cubeMapFbo->isMultiSampled()) {
            blitCubeFace(idx);
        }

        // re-calculate depth values from a cube to spherical model
        if (Settings::instance()->useDepthTexture()) {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
            _cubeMapFbo->bind(false, 1, buffers); // bind no multi-sampled

            _cubeMapFbo->attachCubeMapTexture(_textures.cubeMapColor, idx);
            _cubeMapFbo->attachCubeMapDepthTexture(_textures.cubeMapDepth, idx);

            glViewport(0, 0, _cubemapResolution, _cubemapResolution);
            glScissor(0, 0, _cubemapResolution, _cubemapResolution);
            glEnable(GL_SCISSOR_TEST);

            Engine::clearBuffer();

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

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, _textures.colorSwap);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, _textures.depthSwap);

            _depthCorrectionShader.bind();
            glUniform1i(_shaderLoc.swapColorLoc, 0);
            glUniform1i(_shaderLoc.swapDepthLoc, 1);
            glUniform1f(
                _shaderLoc.swapNearLoc,
                Engine::instance()->_nearClippingPlaneDist
            );
            glUniform1f(_shaderLoc.swapFarLoc, Engine::instance()->_farClippingPlaneDist);

            Engine::instance()->getCurrentWindow().bindVAO();
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            Engine::instance()->getCurrentWindow().unbindVAO();

            ShaderProgram::unbind();

            glDisable(GL_DEPTH_TEST);

            if (alpha) {
                glDisable(GL_BLEND);
            }

            glDepthFunc(GL_LESS);
            glDisable(GL_SCISSOR_TEST);
        }
    };

    internalRender(_subViewports.right, 0);
    internalRender(_subViewports.left, 1);
    internalRender(_subViewports.bottom, 2);
    internalRender(_subViewports.top, 3);
    internalRender(_subViewports.front, 4);
    internalRender(_subViewports.back, 5);
}

void FisheyeProjection::setDomeDiameter(float diameter) {
    _diameter = diameter;
    //generateCubeMapViewports();
}

void FisheyeProjection::setTilt(float angle) {
    _tilt = angle;
}

void FisheyeProjection::setFOV(float angle) {
    _fov = angle;
}

void FisheyeProjection::setRenderingMethod(FisheyeMethod method) {
    _method = method;
}

void FisheyeProjection::setCropFactors(float left, float right, float bottom, float top) {
    _cropFactor.left = glm::clamp(left, 0.f, 1.f);
    _cropFactor.right = glm::clamp(right, 0.f, 1.f);
    _cropFactor.bottom = glm::clamp(bottom, 0.f, 1.f);
    _cropFactor.top = glm::clamp(top, 0.f, 1.f);
}

void FisheyeProjection::setOffset(glm::vec3 offset) {
    _offset = std::move(offset);
    _totalOffset = _baseOffset + _offset;

    _offAxis = glm::length(_totalOffset) > 0.f;
}

void FisheyeProjection::setBaseOffset(glm::vec3 offset) {
    _baseOffset = std::move(offset);
    _totalOffset = _baseOffset + _offset;

    _offAxis = glm::length(_totalOffset) > 0.f;
}

void FisheyeProjection::setIgnoreAspectRatio(bool state) {
    _ignoreAspectRatio = state;
}

glm::vec3 FisheyeProjection::getOffset() const {
    return _totalOffset;
}

void FisheyeProjection::initViewports() {
    // radius is needed to calculate the distance to all view planes
    const float radius = _diameter / 2.f;

    // setup base viewport that will be rotated to create the other cubemap views
    // +Z face
    const glm::vec4 lowerLeftBase(-radius, -radius, radius, 1.f);
    const glm::vec4 upperLeftBase(-radius, radius, radius, 1.f);
    const glm::vec4 upperRightBase(radius, radius, radius, 1.f);

    // 250.5287794 degree FOV covers exactly five sides of a cube, larger FOV needs six
    const float fiveFaceLimit = 2.f * glm::degrees(acos(-1.f / sqrt(3.f)));
    // 109.4712206 degree FOV is needed to cover the entire top face
    const float topFaceLimit = 2.f * glm::degrees(acos(1.f / sqrt(3.f)));


    // four faces doesn't cover more than 180 degrees
    if (_fov > 180.f && _fov <= fiveFaceLimit) {
        _method = FisheyeMethod::FiveFaceCube;
    }

    float cropLevel = 0.5f; // how much of the side faces that are used
    float projectionOffset = 0.f;
    if (_method == FisheyeMethod::FiveFaceCube &&
        _fov >= topFaceLimit && _fov <= fiveFaceLimit)
    {
        float cosAngle = cos(glm::radians(_fov / 2.f));
        float normalizedProjectionOffset = 0.f;
        if (_fov < 180.f) {
            normalizedProjectionOffset = 1.f - _fov / 180.f; // [-1, 0]
        }
        else {
            normalizedProjectionOffset = sqrt((2.f * cosAngle * cosAngle) /
                                              (1.f - cosAngle * cosAngle)); // [0, 1]
        }

        projectionOffset = normalizedProjectionOffset * radius;
        cropLevel = (1.f - normalizedProjectionOffset) / 2.f;
    }
    else if (_fov > fiveFaceLimit) {
        _method = FisheyeMethod::SixFaceCube;
        cropLevel = 0.f;
        projectionOffset = radius;
    }

    const glm::mat4 tiltMat = glm::rotate(
        glm::mat4(1.f),
        glm::radians(90.f - _tilt),
        glm::vec3(1.f, 0.f, 0.f)
    );

    if (_method == FisheyeMethod::FiveFaceCube || _method == FisheyeMethod::SixFaceCube) {
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

            glm::vec4 lowerLeft = lowerLeftBase;
            glm::vec4 upperLeft = upperLeftBase;
            glm::vec4 upperRight = upperRightBase;
            upperRight.x = projectionOffset;

            _subViewports.right.setSize(glm::vec2(1.f - cropLevel, 1.f));

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

        // -X face
        {
            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(90.f),
                glm::vec3(0.f, 1.f, 0.f)
            );

            glm::vec4 lowerLeft = lowerLeftBase;
            lowerLeft.x = -projectionOffset;
            glm::vec4 upperLeft = upperLeftBase;
            upperLeft.x = -projectionOffset;
            glm::vec4 upperRight = upperRightBase;

            _subViewports.left.setPos(glm::vec2(cropLevel, 0.f));
            _subViewports.left.setSize(glm::vec2(1.f - cropLevel, 1.f));

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

        // +Y face
        {
            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(-90.f),
                glm::vec3(1.f, 0.f, 0.f)
            );

            glm::vec4 lowerLeft = lowerLeftBase;
            lowerLeft.y = -projectionOffset;
            glm::vec4 upperLeft = upperLeftBase;
            glm::vec4 upperRight = upperRightBase;

            _subViewports.bottom.setPos(glm::vec2(0.f, cropLevel));
            _subViewports.bottom.setSize(glm::vec2(1.f, 1.f - cropLevel));

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

        // -Y face
        {
            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(90.f),
                glm::vec3(1.f, 0.f, 0.f)
            );

            glm::vec4 lowerLeft = lowerLeftBase;
            glm::vec4 upperLeft = upperLeftBase;
            upperLeft.y = projectionOffset;
            glm::vec4 upperRight = upperRightBase;
            upperRight.y = projectionOffset;

            _subViewports.top.setSize(glm::vec2(1.f, 1.f - cropLevel));

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

        // +Z face
        {
            _subViewports.front.getProjectionPlane().setCoordinateLowerLeft(
                glm::vec3(rollRot * lowerLeftBase)
            );
            _subViewports.front.getProjectionPlane().setCoordinateUpperLeft(
                glm::vec3(rollRot * upperLeftBase)
            );
            _subViewports.front.getProjectionPlane().setCoordinateUpperRight(
                glm::vec3(rollRot * upperRightBase)
            );
        }

        // -Z face
        {
            if (_method == FisheyeMethod::FiveFaceCube) {
                _subViewports.back.setEnabled(false);
            }

            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(180.f),
                glm::vec3(0.f, 1.f, 0.f)
            );

            _subViewports.back.getProjectionPlane().setCoordinateLowerLeft(
                glm::vec3(rotMat * lowerLeftBase)
            );
            _subViewports.back.getProjectionPlane().setCoordinateUpperLeft(
                glm::vec3(rotMat * upperLeftBase)
            );
            _subViewports.back.getProjectionPlane().setCoordinateUpperRight(
                glm::vec3(rotMat * upperRightBase)
            );
        }
    }
    else {
        const glm::mat4 panRot = glm::rotate(
            tiltMat,
            glm::radians(45.f),
            glm::vec3(0.f, 1.f, 0.f)
        );


        // +X face
        {
            const glm::mat4 rotMat = glm::rotate(
                panRot,
                glm::radians(-90.f),
                glm::vec3(0.f, 1.f, 0.f)
            );

            _subViewports.right.getProjectionPlane().setCoordinateLowerLeft(
                glm::vec3(rotMat * lowerLeftBase)
            );
            _subViewports.right.getProjectionPlane().setCoordinateUpperLeft(
                glm::vec3(rotMat * upperLeftBase)
            );
            _subViewports.right.getProjectionPlane().setCoordinateUpperRight(
                glm::vec3(rotMat * upperRightBase)
            );
        }

        // -X face
        {
            _subViewports.left.setEnabled(false);

            const glm::mat4 rotMat = glm::rotate(
                panRot,
                glm::radians(90.f),
                glm::vec3(0.f, 1.f, 0.f)
            );

            _subViewports.left.getProjectionPlane().setCoordinateLowerLeft(
                glm::vec3(rotMat * lowerLeftBase)
            );
            _subViewports.left.getProjectionPlane().setCoordinateUpperLeft(
                glm::vec3(rotMat * upperLeftBase)
            );
            _subViewports.left.getProjectionPlane().setCoordinateUpperRight(
                glm::vec3(rotMat * upperRightBase)
            );
        }

        // +Y face
        {
            const glm::mat4 rotMat = glm::rotate(
                panRot,
                glm::radians(-90.f),
                glm::vec3(1.f, 0.f, 0.f)
            );

            _subViewports.bottom.getProjectionPlane().setCoordinateLowerLeft(
                glm::vec3(rotMat * lowerLeftBase)
            );
            _subViewports.bottom.getProjectionPlane().setCoordinateUpperLeft(
                glm::vec3(rotMat * upperLeftBase)
            );
            _subViewports.bottom.getProjectionPlane().setCoordinateUpperRight(
                glm::vec3(rotMat * upperRightBase)
            );
        }

        // -Y face
        {
            const glm::mat4 rotMat = glm::rotate(
                panRot,
                glm::radians(90.f),
                glm::vec3(1.f, 0.f, 0.f)
            );

            _subViewports.top.getProjectionPlane().setCoordinateLowerLeft(
                glm::vec3(rotMat * lowerLeftBase)
            );
            _subViewports.top.getProjectionPlane().setCoordinateUpperLeft(
                glm::vec3(rotMat * upperLeftBase)
            );
            _subViewports.top.getProjectionPlane().setCoordinateUpperRight(
                glm::vec3(rotMat * upperRightBase)
            );
        }

        // +Z face
        {
            _subViewports.front.getProjectionPlane().setCoordinateLowerLeft(
                glm::vec3(panRot * lowerLeftBase)
            );
            _subViewports.front.getProjectionPlane().setCoordinateUpperLeft(
                glm::vec3(panRot * upperLeftBase)
            );
            _subViewports.front.getProjectionPlane().setCoordinateUpperRight(
                glm::vec3(panRot * upperRightBase)
            );
        }

        // -Z face
        {
            _subViewports.back.setEnabled(false);
            glm::vec4 lowerLeft = lowerLeftBase;
            glm::vec4 upperLeft = upperLeftBase;
            glm::vec4 upperRight = upperRightBase;

            const glm::mat4 rotMat = glm::rotate(
                panRot,
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
}

void FisheyeProjection::initShaders() {
    if (_stereo || _preferedMonoFrustumMode != Frustum::Mode::MonoEye) {
        // if any frustum mode other than Mono (or stereo)
        _offAxis = true;
    }

    // reload shader program if it exists
    if (_shader.isLinked()) {
        _shader.deleteProgram();
    }

    std::string fisheyeFragmentShader;
    std::string fisheyeVertexShader;

    const bool isCubic = (_interpolationMode == InterpolationMode::Cubic);

    // modern pipeline
    fisheyeVertexShader = isCubic ?
        shaders_fisheye_cubic::FisheyeVert :
        shaders_fisheye::FisheyeVert;

    if (_offAxis) {
        if (Settings::instance()->useDepthTexture()) {
            switch (Settings::instance()->getDrawBufferType()) {
                case Settings::DrawBufferType::Diffuse:
                default:
                    fisheyeFragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragOffAxisDepth :
                        shaders_fisheye::FisheyeFragOffAxisDepth;
                    break;
                case Settings::DrawBufferType::DiffuseNormal:
                    fisheyeFragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragOffAxisDepthNormal :
                        shaders_fisheye::FisheyeFragOffAxisDepthNormal;
                    break;
                case Settings::DrawBufferType::DiffusePosition:
                    fisheyeFragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragOffAxisDepthPosition :
                        shaders_fisheye::FisheyeFragOffAxisDepthPosition;
                    break;
                case Settings::DrawBufferType::DiffuseNormalPosition:
                    fisheyeFragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragOffAxisDepthNormalPosition :
                        shaders_fisheye::FisheyeFragOffAxisDepthNormalPosition;
                    break;
            }
        }
        else {
            // no depth
            switch (Settings::instance()->getDrawBufferType()) {
                case Settings::DrawBufferType::Diffuse:
                default:
                    fisheyeFragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragOffAxis :
                        shaders_fisheye::FisheyeFragOffAxis;
                    break;
                case Settings::DrawBufferType::DiffuseNormal:
                    fisheyeFragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragOffAxisNormal :
                        shaders_fisheye::FisheyeFragOffAxisNormal;
                    break;
                case Settings::DrawBufferType::DiffusePosition:
                    fisheyeFragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragOffAxisPosition :
                        shaders_fisheye::FisheyeFragOffAxisPosition;
                    break;
                case Settings::DrawBufferType::DiffuseNormalPosition:
                    fisheyeFragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragOffAxisNormalPosition :
                        shaders_fisheye::FisheyeFragOffAxisNormalPosition;
                    break;
            }
        }
    }
    else {
        // not off axis
        if (Settings::instance()->useDepthTexture()) {
            switch (Settings::instance()->getDrawBufferType()) {
                case Settings::DrawBufferType::Diffuse:
                default:
                    fisheyeFragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragDepth :
                        shaders_fisheye::FisheyeFragDepth;
                    break;
                case Settings::DrawBufferType::DiffuseNormal:
                    fisheyeFragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragDepthNormal :
                        shaders_fisheye::FisheyeFragDepthNormal;
                    break;
                case Settings::DrawBufferType::DiffusePosition:
                    fisheyeFragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragDepthPosition :
                        shaders_fisheye::FisheyeFragDepthPosition;
                    break;
                case Settings::DrawBufferType::DiffuseNormalPosition:
                    fisheyeFragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragDepthNormalPosition :
                        shaders_fisheye::FisheyeFragDepthNormalPosition;
                    break;
            }
        }
        else {
            // no depth
            switch (Settings::instance()->getDrawBufferType()) {
                case Settings::DrawBufferType::Diffuse:
                default:
                    fisheyeFragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFrag :
                        shaders_fisheye::FisheyeFrag;
                    break;
                case Settings::DrawBufferType::DiffuseNormal:
                    fisheyeFragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragNormal :
                        shaders_fisheye::FisheyeFragNormal;
                    break;
                case Settings::DrawBufferType::DiffusePosition:
                    fisheyeFragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragPosition :
                        shaders_fisheye::FisheyeFragPosition;
                    break;
                case Settings::DrawBufferType::DiffuseNormalPosition:
                    fisheyeFragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragNormalPosition :
                        shaders_fisheye::FisheyeFragNormalPosition;
                    break;
            }
        }
    }

    // depth correction shader only
    if (Settings::instance()->useDepthTexture()) {
        std::string depthCorrFragShader = shaders_fisheye::BaseVert;
        std::string depthCorrVertShader = shaders_fisheye::FisheyeDepthCorrectionFrag;

        const std::string glsl = Engine::instance()->getGLSLVersion();
        // replace glsl version
        helpers::findAndReplace(depthCorrFragShader, "**glsl_version**", glsl);
        helpers::findAndReplace(depthCorrVertShader, "**glsl_version**", glsl);

        bool depthCorrFrag = _depthCorrectionShader.addShaderSrc(
            depthCorrFragShader,
            GL_VERTEX_SHADER,
            ShaderProgram::ShaderSourceType::String
        );
        if (!depthCorrFrag) {
            MessageHandler::printError(
                "Failed to load fisheye depth correction vertex shader"
            );
        }
        bool depthCorrVert = _depthCorrectionShader.addShaderSrc(
            depthCorrVertShader,
            GL_FRAGMENT_SHADER,
            ShaderProgram::ShaderSourceType::String
        );
        if (!depthCorrVert) {
            MessageHandler::printError(
                "Failed to load fisheye depth correction fragment shader"
            );
        }
    }

    // add functions to shader
    if (_offAxis) {
        helpers::findAndReplace(
            fisheyeFragmentShader,
            "**sample_fun**",
            shaders_fisheye::SampleOffsetFun
        );
    }
    else {
        helpers::findAndReplace(
            fisheyeFragmentShader,
            "**sample_fun**",
            shaders_fisheye::SampleFun
        );
    }

    if (isCubic) {
        // add functions to shader
        helpers::findAndReplace(
            fisheyeFragmentShader, "**cubic_fun**",
            shaders_fisheye_cubic::catmullRomFun
        );
        helpers::findAndReplace(
            fisheyeFragmentShader, "**interpolatef**",
            shaders_fisheye_cubic::interpolate4_f
        );
        helpers::findAndReplace(
            fisheyeFragmentShader, "**interpolate3f**",
            shaders_fisheye_cubic::interpolate4_3f
        );
        helpers::findAndReplace(
            fisheyeFragmentShader, "**interpolate4f**",
            shaders_fisheye_cubic::interpolate4_4f
        );

        helpers::findAndReplace(
            fisheyeFragmentShader,
            "**size**",
            std::to_string(_cubemapResolution) + ".0"
        );

        helpers::findAndReplace(fisheyeFragmentShader, "**step**", "1.0");
    }

    // replace add correct transform in the fragment shader
    if (_method == FisheyeMethod::FourFaceCube) {
        helpers::findAndReplace(
            fisheyeFragmentShader,
            "**rotVec**",
            "vec3 rotVec = vec3(angle45Factor*x + angle45Factor*z, y, "
            "-angle45Factor*x + angle45Factor*z)"
        );
    }
    else {
        // five or six face
        helpers::findAndReplace(
            fisheyeFragmentShader,
            "**rotVec**",
            "vec3 rotVec = vec3(angle45Factor*x - angle45Factor*y, "
            "angle45Factor*x + angle45Factor*y, z)"
        );
    }

    // replace glsl version
    helpers::findAndReplace(
        fisheyeVertexShader,
        "**glsl_version**",
        Engine::instance()->getGLSLVersion()
    );
    helpers::findAndReplace(
        fisheyeFragmentShader,
        "**glsl_version**",
        Engine::instance()->getGLSLVersion()
    );

    // replace color
    std::stringstream ssColor;
    ssColor.precision(2);
    ssColor << std::fixed << "vec4(" << _clearColor.r << ", " << _clearColor.g
            << ", " << _clearColor.b << ", " << _clearColor.a << ")";
    helpers::findAndReplace(fisheyeFragmentShader, "**bgColor**", ssColor.str());

    bool fisheyeVertex = _shader.addShaderSrc(
        fisheyeVertexShader,
        GL_VERTEX_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!fisheyeVertex) {
        MessageHandler::printError(
            "Failed to load fisheye vertex shader: %s", fisheyeVertexShader.c_str()
        );
    }
    bool fisheyeFragment = _shader.addShaderSrc(
        fisheyeFragmentShader,
        GL_FRAGMENT_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!fisheyeFragment) {
        MessageHandler::printError(
            "Failed to load fisheye fragment shader %s", fisheyeFragmentShader.c_str()
        );
    }

    _shader.setName("FisheyeShader");
    _shader.createAndLinkProgram();
    _shader.bind();

    _shaderLoc.cubemapLoc = _shader.getUniformLocation("cubemap");
    glUniform1i(_shaderLoc.cubemapLoc, 0);

    if (Settings::instance()->useDepthTexture()) {
        _shaderLoc.depthCubemapLoc = _shader.getUniformLocation("depthmap");
        glUniform1i(_shaderLoc.depthCubemapLoc, 1);
    }

    if (Settings::instance()->useNormalTexture()) {
        _shaderLoc.normalCubemapLoc = _shader.getUniformLocation("normalmap");
        glUniform1i(_shaderLoc.normalCubemapLoc, 2);
    }
    
    if (Settings::instance()->usePositionTexture()) {
        _shaderLoc.positionCubemapLoc = _shader.getUniformLocation("positionmap");
        glUniform1i(_shaderLoc.positionCubemapLoc, 3);
    }

    _shaderLoc.halfFovLoc = _shader.getUniformLocation("halfFov");
    glUniform1f(_shaderLoc.halfFovLoc, glm::half_pi<float>());

    if (_offAxis) {
        _shaderLoc.offsetLoc = _shader.getUniformLocation("offset");
        glUniform3f(_shaderLoc.offsetLoc, _totalOffset.x, _totalOffset.y, _totalOffset.z);
    }

    ShaderProgram::unbind();

    if (Settings::instance()->useDepthTexture()) {
        _depthCorrectionShader.setName("FisheyeDepthCorrectionShader");
        _depthCorrectionShader.createAndLinkProgram();
        _depthCorrectionShader.bind();

        _shaderLoc.swapColorLoc = _depthCorrectionShader.getUniformLocation("cTex");
        glUniform1i(_shaderLoc.swapColorLoc, 0);
        _shaderLoc.swapDepthLoc = _depthCorrectionShader.getUniformLocation("dTex");
        glUniform1i(_shaderLoc.swapDepthLoc, 1);
        _shaderLoc.swapNearLoc = _depthCorrectionShader.getUniformLocation("near");
        _shaderLoc.swapFarLoc = _depthCorrectionShader.getUniformLocation("far");

        ShaderProgram::unbind();
    }
}

void FisheyeProjection::drawCubeFace(BaseViewport& face) {
    glLineWidth(1.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glDepthFunc(GL_LESS);

    glEnable(GL_SCISSOR_TEST);
    setupViewport(face);

    Engine::clearBuffer();

    glDisable(GL_SCISSOR_TEST);

    Engine::instance()->_drawFn();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void FisheyeProjection::blitCubeFace(int face) {
    // copy AA-buffer to "regular"/non-AA buffer
    // bind separate read and draw buffers to prepare blit operation
    _cubeMapFbo->bindBlit();
    attachTextures(face);
    _cubeMapFbo->blit();
}

void FisheyeProjection::attachTextures(int face) {
    if (Settings::instance()->useDepthTexture()) {
        _cubeMapFbo->attachDepthTexture(_textures.depthSwap);
        _cubeMapFbo->attachColorTexture(_textures.colorSwap);
    }
    else {
        _cubeMapFbo->attachCubeMapTexture(_textures.cubeMapColor, face);
    }

    if (Settings::instance()->useNormalTexture()) {
        _cubeMapFbo->attachCubeMapTexture(
            _textures.cubeMapNormals,
            face,
            GL_COLOR_ATTACHMENT1
        );
    }

    if (Settings::instance()->usePositionTexture()) {
        _cubeMapFbo->attachCubeMapTexture(
            _textures.cubeMapPositions,
            face,
            GL_COLOR_ATTACHMENT2
        );
    }
}

} // namespace sgct::core
