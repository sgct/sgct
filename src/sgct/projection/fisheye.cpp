/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection/fisheye.h>

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/profiling.h>
#include <sgct/settings.h>
#include <sgct/user.h>
#include <sgct/window.h>
#include <sgct/shaders/internalfisheyeshaders.h>
#include <sgct/shaders/internalfisheyeshaders_cubic.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

namespace sgct {

FisheyeProjection::FisheyeProjection(const Window* parent)
    : NonLinearProjection(parent)
{}

void FisheyeProjection::update(glm::vec2 size) {
    const float cropAspect =
        ((1.f - 2.f * _cropBottom) + (1.f - 2.f * _cropTop)) /
        ((1.f - 2.f * _cropLeft) + (1.f - 2.f * _cropRight));

    const float frameBufferAspect = _ignoreAspectRatio ? 1.f : (size.x / size.y);

    float x = 1.f;
    float y = 1.f;
    if (_keepAspectRatio) {
        float aspect = frameBufferAspect * cropAspect;
        if (aspect >= 1.f) {
            x = 1.f / aspect;
        }
        else {
            y = aspect;
        }
    }

    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    const std::array<const float, 20> v = {
              _cropLeft,        _cropBottom, -x, -y, -1.f,
              _cropLeft,  1.f - _cropTop,    -x,  y, -1.f,
        1.f - _cropRight,       _cropBottom,  x, -y, -1.f,
        1.f - _cropRight, 1.f - _cropTop,     x,  y, -1.f
    };
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void FisheyeProjection::render(const Window& window, const BaseViewport& viewport,
                               Frustum::Mode frustumMode)
{
    ZoneScoped

    glEnable(GL_SCISSOR_TEST);
    Engine::instance().setupViewport(window, viewport, frustumMode);
    glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    _shader.bind();

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // if for some reson the active texture has been reset
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapColor);

    if (Settings::instance().useDepthTexture()) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapDepth);
        glUniform1i(_shaderLoc.depthCubemap, 1);
    }

    if (Settings::instance().useNormalTexture()) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapNormals);
        glUniform1i(_shaderLoc.normalCubemap, 2);
    }

    if (Settings::instance().usePositionTexture()) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapPositions);
        glUniform1i(_shaderLoc.positionCubemap, 3);
    }

    glDisable(GL_CULL_FACE);
    const bool hasAlpha = window.hasAlpha();
    if (hasAlpha) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else {
        glDisable(GL_BLEND);
    }
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    glUniform1i(_shaderLoc.cubemap, 0);
    glUniform1f(_shaderLoc.halfFov, glm::radians<float>(_fov / 2.f));

    if (_isOffAxis) {
        glUniform3fv(_shaderLoc.offset, 1, glm::value_ptr(_totalOffset));
    }

    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    ShaderProgram::unbind();

    glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glDisable(GL_DEPTH_TEST);

    if (hasAlpha) {
        glDisable(GL_BLEND);
    }

    // restore depth func
    glDepthFunc(GL_LESS);
}

void FisheyeProjection::renderCubemap(Window& window, Frustum::Mode frustumMode) {
    ZoneScoped

    switch (frustumMode) {
        case Frustum::Mode::MonoEye:
            break;
        case Frustum::Mode::StereoLeftEye:
            setOffset(
                glm::vec3(-Engine::defaultUser().eyeSeparation() / _diameter, 0.f, 0.f)
            );
            break;
        case Frustum::Mode::StereoRightEye:
            setOffset(
                glm::vec3(Engine::defaultUser().eyeSeparation() / _diameter, 0.f, 0.f)
            );
            break;
        default: throw std::logic_error("Unhandled case label");
    }

    auto render = [this](const Window& win, BaseViewport& vp, int idx, Frustum::Mode mode)
    {
        if (!vp.isEnabled()) {
            return;
        }

        _cubeMapFbo->bind();
        if (!_cubeMapFbo->isMultiSampled()) {
            attachTextures(idx);
        }

        RenderData renderData(
            win,
            vp,
            mode,
            ClusterManager::instance().sceneTransform(),
            vp.projection(mode).viewMatrix(),
            vp.projection(mode).projectionMatrix(),
            vp.projection(mode).viewProjectionMatrix() *
                ClusterManager::instance().sceneTransform()
        );
        drawCubeFace(vp, renderData);

        // blit MSAA fbo to texture
        if (_cubeMapFbo->isMultiSampled()) {
            blitCubeFace(idx);
        }

        // re-calculate depth values from a cube to spherical model
        if (Settings::instance().useDepthTexture()) {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
            _cubeMapFbo->bind(false, 1, buffers); // bind no multi-sampled

            _cubeMapFbo->attachCubeMapTexture(
                _textures.cubeMapColor,
                idx,
                GL_COLOR_ATTACHMENT0
            );
            _cubeMapFbo->attachCubeMapDepthTexture(_textures.cubeMapDepth, idx);

            glViewport(0, 0, _cubemapResolution, _cubemapResolution);
            glScissor(0, 0, _cubemapResolution, _cubemapResolution);
            glEnable(GL_SCISSOR_TEST);

            const glm::vec4 color = Engine::instance().clearColor();
            const bool hasAlpha = win.hasAlpha();
            glClearColor(color.r, color.g, color.b, hasAlpha ? 0.f : color.a);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glDisable(GL_CULL_FACE);
            if (hasAlpha) {
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
            glUniform1i(_shaderLoc.swapColor, 0);
            glUniform1i(_shaderLoc.swapDepth, 1);
            glUniform1f(_shaderLoc.swapNear, Engine::instance().nearClipPlane());
            glUniform1f(_shaderLoc.swapFar, Engine::instance().farClipPlane());

            win.renderScreenQuad();
            ShaderProgram::unbind();

            glDisable(GL_DEPTH_TEST);

            if (hasAlpha) {
                glDisable(GL_BLEND);
            }

            glDepthFunc(GL_LESS);
            glDisable(GL_SCISSOR_TEST);
        }
    };

    render(window, _subViewports.right, 0, frustumMode);
    render(window, _subViewports.left, 1, frustumMode);
    render(window, _subViewports.bottom, 2, frustumMode);
    render(window, _subViewports.top, 3, frustumMode);
    render(window, _subViewports.front, 4, frustumMode);
    render(window, _subViewports.back, 5, frustumMode);
}

void FisheyeProjection::setDomeDiameter(float diameter) {
    _diameter = diameter;
}

void FisheyeProjection::setTilt(float angle) {
    _tilt = angle;
}

void FisheyeProjection::setFOV(float angle) {
    _fov = angle;
}

void FisheyeProjection::setCropFactors(float left, float right, float bottom, float top) {
    _cropLeft = glm::clamp(left, 0.f, 1.f);
    _cropRight = glm::clamp(right, 0.f, 1.f);
    _cropBottom = glm::clamp(bottom, 0.f, 1.f);
    _cropTop = glm::clamp(top, 0.f, 1.f);
}

void FisheyeProjection::setOffset(glm::vec3 offset) {
    _offset = std::move(offset);
    _totalOffset = _baseOffset + _offset;
    _isOffAxis = glm::length(_totalOffset) > 0.f;
}

void FisheyeProjection::setBaseOffset(glm::vec3 offset) {
    _baseOffset = std::move(offset);
    _totalOffset = _baseOffset + _offset;
    _isOffAxis = glm::length(_totalOffset) > 0.f;
}

void FisheyeProjection::setIgnoreAspectRatio(bool state) {
    _ignoreAspectRatio = state;
}

void FisheyeProjection::setKeepAspectRatio(bool state) {
    _keepAspectRatio = state;
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
        const float cosAngle = cos(glm::radians(_fov / 2.f));
        const float normalizedProjectionOffset =
            _fov < 180.f ?
                1.f - _fov / 180.f :
                sqrt((2.f * cosAngle * cosAngle) / (1.f - cosAngle * cosAngle));

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
            _subViewports.right.setSize(glm::vec2(1.f - cropLevel, 1.f));

            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(-90.f),
                glm::vec3(0.f, 1.f, 0.f)
            );

            glm::vec4 upperRight = upperRightBase;
            upperRight.x = projectionOffset;

            _subViewports.right.projectionPlane().setCoordinates(
                glm::vec3(rotMat * lowerLeftBase),
                glm::vec3(rotMat * upperLeftBase),
                glm::vec3(rotMat * upperRight)
            );
        }

        // -X face
        {
            _subViewports.left.setPos(glm::vec2(cropLevel, 0.f));
            _subViewports.left.setSize(glm::vec2(1.f - cropLevel, 1.f));

            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(90.f),
                glm::vec3(0.f, 1.f, 0.f)
            );

            glm::vec4 lowerLeft = lowerLeftBase;
            lowerLeft.x = -projectionOffset;
            glm::vec4 upperLeft = upperLeftBase;
            upperLeft.x = -projectionOffset;

            _subViewports.left.projectionPlane().setCoordinates(
                glm::vec3(rotMat * lowerLeft),
                glm::vec3(rotMat * upperLeft),
                glm::vec3(rotMat * upperRightBase)
            );
        }

        // +Y face
        {
            _subViewports.bottom.setPos(glm::vec2(0.f, cropLevel));
            _subViewports.bottom.setSize(glm::vec2(1.f, 1.f - cropLevel));

            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(-90.f),
                glm::vec3(1.f, 0.f, 0.f)
            );

            glm::vec4 lowerLeft = lowerLeftBase;
            lowerLeft.y = -projectionOffset;

            _subViewports.bottom.projectionPlane().setCoordinates(
                glm::vec3(rotMat * lowerLeft),
                glm::vec3(rotMat * upperLeftBase),
                glm::vec3(rotMat * upperRightBase)
            );
        }

        // -Y face
        {
            _subViewports.top.setSize(glm::vec2(1.f, 1.f - cropLevel));

            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(90.f),
                glm::vec3(1.f, 0.f, 0.f)
            );

            glm::vec4 upperLeft = upperLeftBase;
            upperLeft.y = projectionOffset;
            glm::vec4 upperRight = upperRightBase;
            upperRight.y = projectionOffset;

            _subViewports.top.projectionPlane().setCoordinates(
                glm::vec3(rotMat * lowerLeftBase),
                glm::vec3(rotMat * upperLeft),
                glm::vec3(rotMat * upperRight)
            );
        }

        // +Z face
        {
            _subViewports.front.projectionPlane().setCoordinates(
                glm::vec3(rollRot * lowerLeftBase),
                glm::vec3(rollRot * upperLeftBase),
                glm::vec3(rollRot * upperRightBase)
            );
        }

        // -Z face
        {
            if (_method == FisheyeMethod::FiveFaceCube) {
                _subViewports.back.setEnabled(false);
            }
            else {
                const glm::mat4 rotMat = glm::rotate(
                    rollRot,
                    glm::radians(180.f),
                    glm::vec3(0.f, 1.f, 0.f)
                );

                _subViewports.back.projectionPlane().setCoordinates(
                    glm::vec3(rotMat * lowerLeftBase),
                    glm::vec3(rotMat * upperLeftBase),
                    glm::vec3(rotMat * upperRightBase)
                );
            }
        }
    }
    else {
        // FourFaceCube
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

            _subViewports.right.projectionPlane().setCoordinates(
                glm::vec3(rotMat * lowerLeftBase),
                glm::vec3(rotMat * upperLeftBase),
                glm::vec3(rotMat * upperRightBase)
            );
        }

        // -X face
        _subViewports.left.setEnabled(false);

        // +Y face
        {
            const glm::mat4 rotMat = glm::rotate(
                panRot,
                glm::radians(-90.f),
                glm::vec3(1.f, 0.f, 0.f)
            );

            _subViewports.bottom.projectionPlane().setCoordinates(
                glm::vec3(rotMat * lowerLeftBase),
                glm::vec3(rotMat * upperLeftBase),
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

            _subViewports.top.projectionPlane().setCoordinates(
                glm::vec3(rotMat * lowerLeftBase),
                glm::vec3(rotMat * upperLeftBase),
                glm::vec3(rotMat * upperRightBase)
            );
        }

        // +Z face
        {
            _subViewports.front.projectionPlane().setCoordinates(
                glm::vec3(panRot * lowerLeftBase),
                glm::vec3(panRot * upperLeftBase),
                glm::vec3(panRot * upperRightBase)
            );
        }

        // -Z face
        _subViewports.back.setEnabled(false);
    }
}

void FisheyeProjection::initShaders() {
    if (_isStereo || _preferedMonoFrustumMode != Frustum::Mode::MonoEye) {
        // if any frustum mode other than Mono (or stereo)
        _isOffAxis = true;
    }

    // reload shader program if it exists
    _shader.deleteProgram();

    const bool isCubic = (_interpolationMode == InterpolationMode::Cubic);
    const bool useDepth = Settings::instance().useDepthTexture();
    std::string fragmentShader = [](bool isOffAxis, bool useDepth,
                                    Settings::DrawBufferType t, bool isCubic)
    {
        // It would be nice to do a multidimensional switch statement -.-

        constexpr auto tuple = [](bool isOffAxis, bool useDepth,
                                  Settings::DrawBufferType t, bool isCubic) -> uint16_t
        {
            // Injective mapping from <bool, bool, bool, DrawBufferType> to uint16_t
            uint16_t res = 0;
            res += static_cast<uint8_t>(t);
            if (isCubic) {
                res += 1 << 10;
            }
            if (isOffAxis) {
                res += 1 << 11;
            }
            if (useDepth) {
                res += 1 << 12;
            }
            return res;
        };

        using DrawBufferType = Settings::DrawBufferType;
        switch (tuple(isOffAxis, useDepth, t, isCubic)) {
            case tuple(true, true, DrawBufferType::Diffuse, true):
                return shaders_fisheye_cubic::FisheyeFragOffAxisDepth;
            case tuple(true, true, DrawBufferType::Diffuse, false):
                return shaders_fisheye::FisheyeFragOffAxisDepth;
            case tuple(true, true, DrawBufferType::DiffuseNormal, true):
                return shaders_fisheye_cubic::FisheyeFragOffAxisDepthNormal;
            case tuple(true, true, DrawBufferType::DiffuseNormal, false):
                return shaders_fisheye::FisheyeFragOffAxisDepthNormal;
            case tuple(true, true, DrawBufferType::DiffusePosition, true):
                return shaders_fisheye_cubic::FisheyeFragOffAxisDepthPosition;
            case tuple(true, true, DrawBufferType::DiffusePosition, false):
                return shaders_fisheye::FisheyeFragOffAxisDepthPosition;
            case tuple(true, true, DrawBufferType::DiffuseNormalPosition, true):
                return shaders_fisheye_cubic::FisheyeFragOffAxisDepthNormalPosition;
            case tuple(true, true, DrawBufferType::DiffuseNormalPosition, false):
                return shaders_fisheye::FisheyeFragOffAxisDepthNormalPosition;

            case tuple(true, false, DrawBufferType::Diffuse, true):
                return shaders_fisheye_cubic::FisheyeFragOffAxis;
            case tuple(true, false, DrawBufferType::Diffuse, false):
                return shaders_fisheye::FisheyeFragOffAxis;
            case tuple(true, false, DrawBufferType::DiffuseNormal, true):
                return shaders_fisheye_cubic::FisheyeFragOffAxisNormal;
            case tuple(true, false, DrawBufferType::DiffuseNormal, false):
                return shaders_fisheye::FisheyeFragOffAxisNormal;
            case tuple(true, false, DrawBufferType::DiffusePosition, true):
                return shaders_fisheye_cubic::FisheyeFragOffAxisPosition;
            case tuple(true, false, DrawBufferType::DiffusePosition, false):
                return shaders_fisheye::FisheyeFragOffAxisPosition;
            case tuple(true, false, DrawBufferType::DiffuseNormalPosition, true):
                return shaders_fisheye_cubic::FisheyeFragOffAxisNormalPosition;
            case tuple(true, false, DrawBufferType::DiffuseNormalPosition, false):
                return shaders_fisheye::FisheyeFragOffAxisNormalPosition;

            case tuple(false, true, DrawBufferType::Diffuse, true):
                return shaders_fisheye_cubic::FisheyeFragDepth;
            case tuple(false, true, DrawBufferType::Diffuse, false):
                return shaders_fisheye::FisheyeFragDepth;
            case tuple(false, true, DrawBufferType::DiffuseNormal, true):
                return shaders_fisheye_cubic::FisheyeFragDepthNormal;
            case tuple(false, true, DrawBufferType::DiffuseNormal, false):
                return shaders_fisheye::FisheyeFragDepthNormal;
            case tuple(false, true, DrawBufferType::DiffusePosition, true):
                return shaders_fisheye_cubic::FisheyeFragDepthPosition;
            case tuple(false, true, DrawBufferType::DiffusePosition, false):
                return shaders_fisheye::FisheyeFragDepthPosition;
            case tuple(false, true, DrawBufferType::DiffuseNormalPosition, true):
                return shaders_fisheye_cubic::FisheyeFragDepthNormalPosition;
            case tuple(false, true, DrawBufferType::DiffuseNormalPosition, false):
                return shaders_fisheye::FisheyeFragDepthNormalPosition;

            case tuple(false, false, DrawBufferType::Diffuse, true):
                return shaders_fisheye_cubic::FisheyeFrag;
            case tuple(false, false, DrawBufferType::Diffuse, false):
                return shaders_fisheye::FisheyeFrag;
            case tuple(false, false, DrawBufferType::DiffuseNormal, true):
                return shaders_fisheye_cubic::FisheyeFragNormal;
            case tuple(false, false, DrawBufferType::DiffuseNormal, false):
                return shaders_fisheye::FisheyeFragNormal;
            case tuple(false, false, DrawBufferType::DiffusePosition, true):
                return shaders_fisheye_cubic::FisheyeFragPosition;
            case tuple(false, false, DrawBufferType::DiffusePosition, false):
                return shaders_fisheye::FisheyeFragPosition;
            case tuple(false, false, DrawBufferType::DiffuseNormalPosition, true):
                return shaders_fisheye_cubic::FisheyeFragNormalPosition;
            case tuple(false, false, DrawBufferType::DiffuseNormalPosition, false):
                return shaders_fisheye::FisheyeFragNormalPosition;

            default:
                throw std::logic_error("Unhandled case label");
        }
    }(_isOffAxis, useDepth, Settings::instance().drawBufferType(), isCubic);

    std::string samplerShader = 
        _isOffAxis ? shaders_fisheye::SampleOffsetFun : shaders_fisheye::SampleFun;

    _shader = ShaderProgram("FisheyeShader");
    _shader.addShaderSource(shaders_fisheye::FisheyeVert, fragmentShader);
    _shader.addShaderSource(samplerShader, GL_FRAGMENT_SHADER);
    if (isCubic) {
        _shader.addShaderSource(shaders_fisheye_cubic::interpolate, GL_FRAGMENT_SHADER);
    }
    if (_method == FisheyeMethod::FourFaceCube) {
        _shader.addShaderSource(
            shaders_fisheye::RotationFourFaceCubeFun,
            GL_FRAGMENT_SHADER
        );
    }
    else {
        // five or six face
        _shader.addShaderSource(
            shaders_fisheye::RotationFiveSixFaceCubeFun,
            GL_FRAGMENT_SHADER
        );
    }


    _shader.createAndLinkProgram();
    _shader.bind();


    glUniform4fv(
        glGetUniformLocation(_shader.id(), "bgColor"),
        1,
        glm::value_ptr(_clearColor)
    );
    if (isCubic) {
        glUniform1f(glGetUniformLocation(_shader.id(), "size"), _cubemapResolution);
    }

    _shaderLoc.cubemap = glGetUniformLocation(_shader.id(), "cubemap");
    glUniform1i(_shaderLoc.cubemap, 0);

    if (Settings::instance().useDepthTexture()) {
        _shaderLoc.depthCubemap = glGetUniformLocation(_shader.id(), "depthmap");
        glUniform1i(_shaderLoc.depthCubemap, 1);
    }

    if (Settings::instance().useNormalTexture()) {
        _shaderLoc.normalCubemap = glGetUniformLocation(_shader.id(), "normalmap");
        glUniform1i(_shaderLoc.normalCubemap, 2);
    }

    if (Settings::instance().usePositionTexture()) {
        _shaderLoc.positionCubemap = glGetUniformLocation(_shader.id(), "positionmap");
        glUniform1i(_shaderLoc.positionCubemap, 3);
    }

    _shaderLoc.halfFov = glGetUniformLocation(_shader.id(), "halfFov");
    glUniform1f(_shaderLoc.halfFov, glm::half_pi<float>());

    if (_isOffAxis) {
        _shaderLoc.offset = glGetUniformLocation(_shader.id(), "offset");
        glUniform3f(_shaderLoc.offset, _totalOffset.x, _totalOffset.y, _totalOffset.z);
    }

    ShaderProgram::unbind();

    if (Settings::instance().useDepthTexture()) {
        _depthCorrectionShader = ShaderProgram("FisheyeDepthCorrectionShader");
        _depthCorrectionShader.addShaderSource(
            shaders_fisheye::BaseVert,
            shaders_fisheye::FisheyeDepthCorrectionFrag
        );
        _depthCorrectionShader.createAndLinkProgram();
        _depthCorrectionShader.bind();

        _shaderLoc.swapColor = glGetUniformLocation(_depthCorrectionShader.id(), "cTex");
        glUniform1i(_shaderLoc.swapColor, 0);
        _shaderLoc.swapDepth = glGetUniformLocation(_depthCorrectionShader.id(), "dTex");
        glUniform1i(_shaderLoc.swapDepth, 1);
        _shaderLoc.swapNear = glGetUniformLocation(_depthCorrectionShader.id(), "near");
        _shaderLoc.swapFar = glGetUniformLocation(_depthCorrectionShader.id(), "far");

        ShaderProgram::unbind();
    }
}

void FisheyeProjection::drawCubeFace(BaseViewport& face, RenderData renderData) {
    glLineWidth(1.f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glDepthFunc(GL_LESS);

    glEnable(GL_SCISSOR_TEST);
    setupViewport(face);

    const glm::vec4 color = Engine::instance().clearColor();
    const float alpha = renderData.window.hasAlpha() ? 0.f : color.a;
    glClearColor(color.r, color.g, color.b, alpha);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_SCISSOR_TEST);

    Engine::instance().drawFunction()(renderData);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void FisheyeProjection::blitCubeFace(int face) {
    // copy AA-buffer to "regular"/non-AA buffer
    _cubeMapFbo->bindBlit();
    attachTextures(face);
    _cubeMapFbo->blit();
}

void FisheyeProjection::attachTextures(int face) {
    if (Settings::instance().useDepthTexture()) {
        _cubeMapFbo->attachDepthTexture(_textures.depthSwap);
        _cubeMapFbo->attachColorTexture(_textures.colorSwap, GL_COLOR_ATTACHMENT0);
    }
    else {
        _cubeMapFbo->attachCubeMapTexture(
            _textures.cubeMapColor,
            face,
            GL_COLOR_ATTACHMENT0
        );
    }

    if (Settings::instance().useNormalTexture()) {
        _cubeMapFbo->attachCubeMapTexture(
            _textures.cubeMapNormals,
            face,
            GL_COLOR_ATTACHMENT1
        );
    }

    if (Settings::instance().usePositionTexture()) {
        _cubeMapFbo->attachCubeMapTexture(
            _textures.cubeMapPositions,
            face,
            GL_COLOR_ATTACHMENT2
        );
    }
}

} // namespace sgct
