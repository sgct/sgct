/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/fisheyeprojection.h>

#include <sgct/engine.h>
#include <sgct/settings.h>
#include <sgct/user.h>
#include <sgct/window.h>
#include <sgct/helpers/stringfunctions.h>
#include <sgct/shaders/internalfisheyeshaders.h>
#include <sgct/shaders/internalfisheyeshaders_cubic.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

namespace sgct::core {

FisheyeProjection::FisheyeProjection(Window* parent)
    : NonLinearProjection(parent)
{}

void FisheyeProjection::update(glm::vec2 size) {
    const float cropAspect =
        ((1.f - 2.f * _cropFactor.bottom) + (1.f - 2.f * _cropFactor.top)) /
        ((1.f - 2.f * _cropFactor.left) + (1.f - 2.f * _cropFactor.right));

    const float frameBufferAspect = _ignoreAspectRatio ? 1.f : (size.x / size.y);

    float x = 1.f;
    float y = 1.f;
    if (Settings::instance().getTryKeepAspectRatio()) {
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
              _cropFactor.left,        _cropFactor.bottom, -x, -y, -1.f,
              _cropFactor.left,  1.f - _cropFactor.top,    -x,  y, -1.f,
        1.f - _cropFactor.right,       _cropFactor.bottom,  x, -y, -1.f,
        1.f - _cropFactor.right, 1.f - _cropFactor.top,     x,  y, -1.f
    };
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void FisheyeProjection::render() {
    glEnable(GL_SCISSOR_TEST);
    Engine::instance().enterCurrentViewport();
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
        glUniform1i(_shaderLoc.depthCubemapLoc, 1);
    }

    if (Settings::instance().useNormalTexture()) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapNormals);
        glUniform1i(_shaderLoc.normalCubemapLoc, 2);
    }

    if (Settings::instance().usePositionTexture()) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapPositions);
        glUniform1i(_shaderLoc.positionCubemapLoc, 3);
    }

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

    glUniform1i(_shaderLoc.cubemapLoc, 0);
    glUniform1f(_shaderLoc.halfFovLoc, glm::radians<float>(_fov / 2.f));

    if (_isOffAxis) {
        glUniform3fv(_shaderLoc.offsetLoc, 1, glm::value_ptr(_totalOffset));
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

void FisheyeProjection::renderCubemap() {
    switch (Engine::instance().getCurrentFrustumMode()) {
        case Frustum::Mode::MonoEye:
            break;
        case Frustum::Mode::StereoLeftEye:
            setOffset(glm::vec3(
                -Engine::getDefaultUser().getEyeSeparation() / _diameter,
                0.f,
                0.f
            ));
            break;
        case Frustum::Mode::StereoRightEye:
            setOffset(glm::vec3(
                Engine::getDefaultUser().getEyeSeparation() / _diameter,
                0.f,
                0.f
            ));
            break;
        default:
            throw std::logic_error("Unhandled case label");
    }

    auto internalRender = [this](BaseViewport& vp, int idx) {
        if (!vp.isEnabled()) {
            return;
        }

        _cubeMapFbo->bind();
        if (!_cubeMapFbo->isMultiSampled()) {
            attachTextures(idx);
        }

        Engine::instance().getCurrentWindow().setCurrentViewport(&vp);
        drawCubeFace(vp);

        // blit MSAA fbo to texture
        if (_cubeMapFbo->isMultiSampled()) {
            blitCubeFace(idx);
        }

        // re-calculate depth values from a cube to spherical model
        if (Settings::instance().useDepthTexture()) {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
            _cubeMapFbo->bind(false, 1, buffers); // bind no multi-sampled

            _cubeMapFbo->attachCubeMapTexture(_textures.cubeMapColor, idx);
            _cubeMapFbo->attachCubeMapDepthTexture(_textures.cubeMapDepth, idx);

            glViewport(0, 0, _cubemapResolution, _cubemapResolution);
            glScissor(0, 0, _cubemapResolution, _cubemapResolution);
            glEnable(GL_SCISSOR_TEST);

            const glm::vec4 color = Engine::instance().getClearColor();
            const bool hasAlpha = Engine::instance().getCurrentWindow().hasAlpha();
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
            glUniform1i(_shaderLoc.swapColorLoc, 0);
            glUniform1i(_shaderLoc.swapDepthLoc, 1);
            glUniform1f(_shaderLoc.swapNearLoc, Engine::instance().getNearClipPlane());
            glUniform1f(_shaderLoc.swapFarLoc, Engine::instance().getFarClipPlane());

            Engine::instance().getCurrentWindow().renderScreenQuad();
            ShaderProgram::unbind();

            glDisable(GL_DEPTH_TEST);

            if (hasAlpha) {
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

            _subViewports.right.getProjectionPlane().setCoordinates(
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

            _subViewports.left.getProjectionPlane().setCoordinates(
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

            _subViewports.bottom.getProjectionPlane().setCoordinates(
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

            _subViewports.top.getProjectionPlane().setCoordinates(
                glm::vec3(rotMat * lowerLeftBase),
                glm::vec3(rotMat * upperLeft),
                glm::vec3(rotMat * upperRight)
            );
        }

        // +Z face
        {
            _subViewports.front.getProjectionPlane().setCoordinates(
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

                _subViewports.back.getProjectionPlane().setCoordinates(
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

            _subViewports.right.getProjectionPlane().setCoordinates(
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

            _subViewports.bottom.getProjectionPlane().setCoordinates(
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

            _subViewports.top.getProjectionPlane().setCoordinates(
                glm::vec3(rotMat * lowerLeftBase),
                glm::vec3(rotMat * upperLeftBase),
                glm::vec3(rotMat * upperRightBase)
            );
        }

        // +Z face
        {
            _subViewports.front.getProjectionPlane().setCoordinates(
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
    if (_shader.isLinked()) {
        _shader.deleteProgram();
    }

    std::string fragmentShader;

    const bool isCubic = (_interpolationMode == InterpolationMode::Cubic);
    // (abock, 2019-10-15) I tried making this a bit prettier, but I failed
    if (_isOffAxis) {
        if (Settings::instance().useDepthTexture()) {
            switch (Settings::instance().getDrawBufferType()) {
                case Settings::DrawBufferType::Diffuse:
                    fragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragOffAxisDepth :
                        shaders_fisheye::FisheyeFragOffAxisDepth;
                    break;
                case Settings::DrawBufferType::DiffuseNormal:
                    fragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragOffAxisDepthNormal :
                        shaders_fisheye::FisheyeFragOffAxisDepthNormal;
                    break;
                case Settings::DrawBufferType::DiffusePosition:
                    fragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragOffAxisDepthPosition :
                        shaders_fisheye::FisheyeFragOffAxisDepthPosition;
                    break;
                case Settings::DrawBufferType::DiffuseNormalPosition:
                    fragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragOffAxisDepthNormalPosition :
                        shaders_fisheye::FisheyeFragOffAxisDepthNormalPosition;
                    break;
                default:
                    throw std::logic_error("Unhandled case label");
            }
        }
        else {
            // no depth
            switch (Settings::instance().getDrawBufferType()) {
                case Settings::DrawBufferType::Diffuse:
                    fragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragOffAxis :
                        shaders_fisheye::FisheyeFragOffAxis;
                    break;
                case Settings::DrawBufferType::DiffuseNormal:
                    fragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragOffAxisNormal :
                        shaders_fisheye::FisheyeFragOffAxisNormal;
                    break;
                case Settings::DrawBufferType::DiffusePosition:
                    fragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragOffAxisPosition :
                        shaders_fisheye::FisheyeFragOffAxisPosition;
                    break;
                case Settings::DrawBufferType::DiffuseNormalPosition:
                    fragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragOffAxisNormalPosition :
                        shaders_fisheye::FisheyeFragOffAxisNormalPosition;
                    break;
                default:
                    throw std::logic_error("Unhandled case label");
            }
        }
    }
    else {
        // not off axis
        if (Settings::instance().useDepthTexture()) {
            switch (Settings::instance().getDrawBufferType()) {
                case Settings::DrawBufferType::Diffuse:
                    fragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragDepth :
                        shaders_fisheye::FisheyeFragDepth;
                    break;
                case Settings::DrawBufferType::DiffuseNormal:
                    fragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragDepthNormal :
                        shaders_fisheye::FisheyeFragDepthNormal;
                    break;
                case Settings::DrawBufferType::DiffusePosition:
                    fragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragDepthPosition :
                        shaders_fisheye::FisheyeFragDepthPosition;
                    break;
                case Settings::DrawBufferType::DiffuseNormalPosition:
                    fragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragDepthNormalPosition :
                        shaders_fisheye::FisheyeFragDepthNormalPosition;
                    break;
                default:
                    throw std::logic_error("Unhandled case label");
            }
        }
        else {
            // no depth
            switch (Settings::instance().getDrawBufferType()) {
                case Settings::DrawBufferType::Diffuse:
                    fragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFrag :
                        shaders_fisheye::FisheyeFrag;
                    break;
                case Settings::DrawBufferType::DiffuseNormal:
                    fragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragNormal :
                        shaders_fisheye::FisheyeFragNormal;
                    break;
                case Settings::DrawBufferType::DiffusePosition:
                    fragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragPosition :
                        shaders_fisheye::FisheyeFragPosition;
                    break;
                case Settings::DrawBufferType::DiffuseNormalPosition:
                    fragmentShader = isCubic ?
                        shaders_fisheye_cubic::FisheyeFragNormalPosition :
                        shaders_fisheye::FisheyeFragNormalPosition;
                    break;
                default:
                    throw std::logic_error("Unhandled case label");
            }
        }
    }

    // add functions to shader
    helpers::findAndReplace(
        fragmentShader,
        "**sample_fun**",
        _isOffAxis ? shaders_fisheye::SampleOffsetFun : shaders_fisheye::SampleFun
    );

    if (isCubic) {
        // add functions to shader
        helpers::findAndReplace(
            fragmentShader,
            "**interpolate**",
            shaders_fisheye_cubic::interpolate
        );

        helpers::findAndReplace(
            fragmentShader,
            "**size**",
            std::to_string(_cubemapResolution) + ".0"
        );
    }

    // replace add correct transform in the fragment shader
    if (_method == FisheyeMethod::FourFaceCube) {
        helpers::findAndReplace(
            fragmentShader,
            "**rotVec**",
            "const float _angle = 0.7071067812;"
            "vec3 rotVec = vec3(_angle*x + _angle*z, y, -_angle*x + _angle*z)"
        );
    }
    else {
        // five or six face
        helpers::findAndReplace(
            fragmentShader,
            "**rotVec**",
            "const float _angle = 0.7071067812;"
            "vec3 rotVec = vec3(_angle*x - _angle*y, _angle*x + _angle*y, z)"
        );
    }

    // replace color
    std::string color = "vec4(" + std::to_string(_clearColor.r) + ',' +
        std::to_string(_clearColor.g) + ',' + std::to_string(_clearColor.b) + ',' +
        std::to_string(_clearColor.a) + ')';
    helpers::findAndReplace(fragmentShader, "**bgColor**", color);

    _shader = ShaderProgram("FisheyeShader");
    _shader.addShaderSource(shaders_fisheye::FisheyeVert, fragmentShader);
    _shader.createAndLinkProgram();
    _shader.bind();

    _shaderLoc.cubemapLoc = glGetUniformLocation(_shader.getId(), "cubemap");
    glUniform1i(_shaderLoc.cubemapLoc, 0);

    if (Settings::instance().useDepthTexture()) {
        _shaderLoc.depthCubemapLoc = glGetUniformLocation(_shader.getId(), "depthmap");
        glUniform1i(_shaderLoc.depthCubemapLoc, 1);
    }

    if (Settings::instance().useNormalTexture()) {
        _shaderLoc.normalCubemapLoc = glGetUniformLocation(_shader.getId(), "normalmap");
        glUniform1i(_shaderLoc.normalCubemapLoc, 2);
    }
    
    if (Settings::instance().usePositionTexture()) {
        _shaderLoc.positionCubemapLoc =
            glGetUniformLocation(_shader.getId(), "positionmap");
        glUniform1i(_shaderLoc.positionCubemapLoc, 3);
    }

    _shaderLoc.halfFovLoc = glGetUniformLocation(_shader.getId(), "halfFov");
    glUniform1f(_shaderLoc.halfFovLoc, glm::half_pi<float>());

    if (_isOffAxis) {
        _shaderLoc.offsetLoc = glGetUniformLocation(_shader.getId(), "offset");
        glUniform3f(_shaderLoc.offsetLoc, _totalOffset.x, _totalOffset.y, _totalOffset.z);
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
        
        _shaderLoc.swapColorLoc =
            glGetUniformLocation(_depthCorrectionShader.getId(), "cTex");
        glUniform1i(_shaderLoc.swapColorLoc, 0);
        _shaderLoc.swapDepthLoc =
            glGetUniformLocation(_depthCorrectionShader.getId(), "dTex");
        glUniform1i(_shaderLoc.swapDepthLoc, 1);
        _shaderLoc.swapNearLoc =
            glGetUniformLocation(_depthCorrectionShader.getId(), "near");
        _shaderLoc.swapFarLoc =
            glGetUniformLocation(_depthCorrectionShader.getId(), "far");

        ShaderProgram::unbind();
    }
}

void FisheyeProjection::drawCubeFace(BaseViewport& face) {
    glLineWidth(1.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glDepthFunc(GL_LESS);

    glEnable(GL_SCISSOR_TEST);
    setupViewport(face);

    const glm::vec4 color = Engine::instance().getClearColor();
    const float alpha = Engine::instance().getCurrentWindow().hasAlpha() ? 0.f : color.a;
    glClearColor(color.r, color.g, color.b, alpha);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_SCISSOR_TEST);

    Engine::instance().getDrawFunction()();

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
        _cubeMapFbo->attachColorTexture(_textures.colorSwap);
    }
    else {
        _cubeMapFbo->attachCubeMapTexture(_textures.cubeMapColor, face);
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

} // namespace sgct::core
