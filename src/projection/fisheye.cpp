/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection/fisheye.h>

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/internalshaders.h>
#include <sgct/log.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <sgct/user.h>
#include <sgct/window.h>

#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

namespace {
    struct Vertex {
        float x;
        float y;
        float z;
        float s;
        float t;
    };
} // namespace

namespace sgct {

FisheyeProjection::FisheyeProjection(const config::FisheyeProjection& config,
                                     const Window& parent, User& user)
    : NonLinearProjection(parent)
    , _fov(config.fov.value_or(180.f))
    , _tilt(config.tilt.value_or(0.f))
    , _diameter(config.diameter.value_or(14.8f))
    , _cropLeft(config.crop ? config.crop->left : 0.f)
    , _cropRight(config.crop ? config.crop->right : 0.f)
    , _cropBottom(config.crop ? config.crop->bottom : 0.f)
    , _cropTop(config.crop ? config.crop->top : 0.f)
    , _keepAspectRatio(config.keepAspectRatio.value_or(true))
{
    setUser(user);

    if (config.quality) {
        setCubemapResolution(*config.quality);
    }
    if (config.interpolation) {
        const NonLinearProjection::InterpolationMode interp =
            [](config::FisheyeProjection::Interpolation i) {
            switch (i) {
                case config::FisheyeProjection::Interpolation::Linear:
                    return NonLinearProjection::InterpolationMode::Linear;
                case config::FisheyeProjection::Interpolation::Cubic:
                    return NonLinearProjection::InterpolationMode::Cubic;
                default: throw std::logic_error("Unhandled case label");
            }
            }(*config.interpolation);
        setInterpolationMode(interp);
    }
    if (config.offset) {
        setBaseOffset(*config.offset);
    }
    _clearColor = config.background.value_or(_clearColor);
    setUseDepthTransformation(true);
}

FisheyeProjection::~FisheyeProjection() {
    glDeleteBuffers(1, &_vbo);
    glDeleteVertexArrays(1, &_vao);
    _shader.deleteProgram();
    _depthCorrectionShader.deleteProgram();
}

void FisheyeProjection::update(const vec2& size) const {
    // do the cropping in the fragment shader and not by changing the vbo

    const float cropAspect =
        ((1.f - 2.f * _cropBottom) + (1.f - 2.f * _cropTop)) /
        ((1.f - 2.f * _cropLeft) + (1.f - 2.f * _cropRight));

    const float frameBufferAspect = _ignoreAspectRatio ? 1.f : (size.x / size.y);

    float x = 1.f;
    float y = 1.f;
    if (_keepAspectRatio) {
        const float aspect = frameBufferAspect * cropAspect;
        if (aspect >= 1.f) {
            x = 1.f / aspect;
        }
        else {
            y = aspect;
        }
    }

    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    const std::array<const Vertex, 4> v = {
        Vertex{ -x, -y, -1.f, _cropLeft, _cropBottom },
        Vertex{ -x,  y, -1.f, _cropLeft, 1.f - _cropTop },
        Vertex{  x, -y, -1.f, 1.f - _cropRight, _cropBottom },
        Vertex{  x,  y, -1.f, 1.f - _cropRight, 1.f - _cropTop }
    };
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(Vertex), v.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void FisheyeProjection::render(const BaseViewport& viewport,
                               FrustumMode frustumMode) const
{
    ZoneScoped;

    glEnable(GL_SCISSOR_TEST);
    viewport.setupViewport(frustumMode);
    glClearColor(_clearColor.x, _clearColor.y, _clearColor.z, _clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    _shader.bind();

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // if for some reson the active texture has been reset
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapColor);

    if (Engine::instance().settings().useDepthTexture) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapDepth);
        glUniform1i(_shaderLoc.depthCubemap, 1);
    }

    if (Engine::instance().settings().useNormalTexture) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapNormals);
        glUniform1i(_shaderLoc.normalCubemap, 2);
    }

    if (Engine::instance().settings().usePositionTexture) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapPositions);
        glUniform1i(_shaderLoc.positionCubemap, 3);
    }

    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    glUniform1i(_shaderLoc.cubemap, 0);
    glUniform1f(_shaderLoc.halfFov, glm::radians(_fov / 2.f));

    if (_isOffAxis) {
        glUniform3fv(_shaderLoc.offset, 1, &_totalOffset.x);
    }

    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    ShaderProgram::unbind();

    glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void FisheyeProjection::renderCubemap(FrustumMode frustumMode) const {
    ZoneScoped;

    switch (frustumMode) {
        case FrustumMode::Mono:
            break;
        case FrustumMode::StereoLeft:
            setOffset(
                vec3{ -Engine::defaultUser().eyeSeparation() / _diameter, 0.f, 0.f }
            );
            break;
        case FrustumMode::StereoRight:
            setOffset(
                vec3{ Engine::defaultUser().eyeSeparation() / _diameter, 0.f, 0.f }
            );
            break;
    }

    auto render = [this](const BaseViewport& vp, int idx, FrustumMode mode) {
        if (!vp.isEnabled()) {
            return;
        }

        renderCubeFace(vp, idx, mode);

        // re-calculate depth values from a cube to spherical model
        if (Engine::instance().settings().useDepthTexture) {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
            _cubeMapFbo->bind(false, 1, buffers); // bind no multi-sampled

            _cubeMapFbo->attachCubeMapTexture(
                _textures.cubeMapColor,
                idx,
                GL_COLOR_ATTACHMENT0
            );
            _cubeMapFbo->attachCubeMapDepthTexture(_textures.cubeMapDepth, idx);

            glViewport(0, 0, _cubemapResolution.x, _cubemapResolution.y);
            glScissor(0, 0, _cubemapResolution.x, _cubemapResolution.y);
            glEnable(GL_SCISSOR_TEST);

            glClearColor(0.f, 0.f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glDisable(GL_CULL_FACE);
            glDisable(GL_BLEND);

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

            vp.window().renderScreenQuad();
            ShaderProgram::unbind();

            glDisable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glDisable(GL_SCISSOR_TEST);
        }
    };

    render(_subViewports.right, 0, frustumMode);
    render(_subViewports.left, 1, frustumMode);
    render(_subViewports.bottom, 2, frustumMode);
    render(_subViewports.top, 3, frustumMode);
    render(_subViewports.front, 4, frustumMode);
    render(_subViewports.back, 5, frustumMode);
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

void FisheyeProjection::setOffset(vec3 offset) const {
    _offset = std::move(offset);
    _totalOffset.x = _baseOffset.x + _offset.x;
    _totalOffset.y = _baseOffset.y + _offset.y;
    _totalOffset.z = _baseOffset.z + _offset.z;
    _isOffAxis = glm::length(glm::make_vec3(&_totalOffset.x)) > 0.f;
}

void FisheyeProjection::setBaseOffset(vec3 offset) {
    _baseOffset = std::move(offset);
    _totalOffset.x = _baseOffset.x + _offset.x;
    _totalOffset.y = _baseOffset.y + _offset.y;
    _totalOffset.z = _baseOffset.z + _offset.z;
    _isOffAxis = glm::length(glm::make_vec3(&_totalOffset.x)) > 0.f;
}

void FisheyeProjection::setIgnoreAspectRatio(bool state) {
    _ignoreAspectRatio = state;
}

void FisheyeProjection::setKeepAspectRatio(bool state) {
    _keepAspectRatio = state;
}

void FisheyeProjection::initVBO() {
    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, s))
    );

    glBindVertexArray(0);
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
    const float fiveFaceLimit = 2.f * glm::degrees(std::acos(-1.f / std::sqrt(3.f)));
    // 109.4712206 degree FOV is needed to cover the entire top face
    const float topFaceLimit = 2.f * glm::degrees(std::acos(1.f / std::sqrt(3.f)));


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
                std::sqrt((2.f * cosAngle * cosAngle) / (1.f - cosAngle * cosAngle));

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
            _subViewports.right.setSize(vec2{ 1.f - cropLevel, 1.f });

            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(-90.f),
                glm::vec3(0.f, 1.f, 0.f)
            );

            glm::vec4 upperRight = upperRightBase;
            upperRight.x = projectionOffset;

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
            _subViewports.left.setPosition(vec2{ cropLevel, 0.f });
            _subViewports.left.setSize(vec2{ 1.f - cropLevel, 1.f });

            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(90.f),
                glm::vec3(0.f, 1.f, 0.f)
            );

            glm::vec4 lowerLeft = lowerLeftBase;
            lowerLeft.x = -projectionOffset;
            glm::vec4 upperLeft = upperLeftBase;
            upperLeft.x = -projectionOffset;

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
            _subViewports.bottom.setPosition(vec2{ 0.f, cropLevel });
            _subViewports.bottom.setSize(vec2{ 1.f, 1.f - cropLevel });

            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(-90.f),
                glm::vec3(1.f, 0.f, 0.f)
            );

            glm::vec4 lowerLeft = lowerLeftBase;
            lowerLeft.y = -projectionOffset;

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
            _subViewports.top.setSize(vec2{ 1.f, 1.f - cropLevel });

            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(90.f),
                glm::vec3(1.f, 0.f, 0.f)
            );

            glm::vec4 upperLeft = upperLeftBase;
            upperLeft.y = projectionOffset;
            glm::vec4 upperRight = upperRightBase;
            upperRight.y = projectionOffset;

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
            if (_method == FisheyeMethod::FiveFaceCube) {
                _subViewports.back.setEnabled(false);
            }
            else {
                const glm::mat4 rotMat = glm::rotate(
                    rollRot,
                    glm::radians(180.f),
                    glm::vec3(0.f, 1.f, 0.f)
                );

                const glm::vec3 ll = glm::vec3(rotMat * lowerLeftBase);
                const glm::vec3 ul = glm::vec3(rotMat * upperLeftBase);
                const glm::vec3 ur = glm::vec3(rotMat * upperRightBase);
                _subViewports.back.projectionPlane().setCoordinates(
                    vec3(ll.x, ll.y, ll.z),
                    vec3(ul.x, ul.y, ul.z),
                    vec3(ur.x, ur.y, ur.z)
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

            const glm::vec3 ll = glm::vec3(rotMat * lowerLeftBase);
            const glm::vec3 ul = glm::vec3(rotMat * upperLeftBase);
            const glm::vec3 ur = glm::vec3(rotMat * upperRightBase);
            _subViewports.right.projectionPlane().setCoordinates(
                vec3(ll.x, ll.y, ll.z),
                vec3(ul.x, ul.y, ul.z),
                vec3(ur.x, ur.y, ur.z)
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

            const glm::vec3 ll = glm::vec3(rotMat * lowerLeftBase);
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
                panRot,
                glm::radians(90.f),
                glm::vec3(1.f, 0.f, 0.f)
            );

            const glm::vec3 ll = glm::vec3(rotMat * lowerLeftBase);
            const glm::vec3 ul = glm::vec3(rotMat * upperLeftBase);
            const glm::vec3 ur = glm::vec3(rotMat * upperRightBase);
            _subViewports.top.projectionPlane().setCoordinates(
                vec3(ll.x, ll.y, ll.z),
                vec3(ul.x, ul.y, ul.z),
                vec3(ur.x, ur.y, ur.z)
            );
        }

        // +Z face
        {
            const glm::vec3 ll = glm::vec3(panRot * lowerLeftBase);
            const glm::vec3 ul = glm::vec3(panRot * upperLeftBase);
            const glm::vec3 ur = glm::vec3(panRot * upperRightBase);
            _subViewports.front.projectionPlane().setCoordinates(
                vec3(ll.x, ll.y, ll.z),
                vec3(ul.x, ul.y, ul.z),
                vec3(ur.x, ur.y, ur.z)
            );
        }

        // -Z face
        _subViewports.back.setEnabled(false);
    }
}

void FisheyeProjection::initShaders() {
    if (_isStereo || _preferedMonoFrustumMode != FrustumMode::Mono) {
        // if any frustum mode other than Mono (or stereo)
        _isOffAxis = true;
    }

    const bool isCubic = (_interpolationMode == InterpolationMode::Cubic);
    const std::string_view fragmentShader = [](bool isOffAxis, bool useDepth,
                                               bool useNormal, bool usePosition)
    {
        // It would be nice to do a multidimensional switch statement -.-

        constexpr auto tuple = [](bool useOffAxis, bool useDepth,
                                  bool useNormal, bool usePosition) -> uint16_t
        {
            // Injective mapping from <bool, bool, bool, bool.> to uint8_t
                uint8_t value = 0;
                value |= useOffAxis ? 0b0000 : 0b0001;
                value |= useDepth ? 0b0000 : 0b0010;
                value |= useNormal ? 0b0000 : 0b0100;
                value |= usePosition ? 0b0000 : 0b1000;
                return value;
        };

        switch (tuple(isOffAxis, useDepth, useNormal, usePosition)) {
            case tuple(true, true, false, false):
                return shaders_fisheye::FisheyeFragOffAxisDepth;
            case tuple(true, true, true, false):
                return shaders_fisheye::FisheyeFragOffAxisDepthNormal;
            case tuple(true, true, false, true):
                return shaders_fisheye::FisheyeFragOffAxisDepthPosition;
            case tuple(true, true, true, true):
                return shaders_fisheye::FisheyeFragOffAxisDepthNormalPosition;

            case tuple(true, false, false, false):
                return shaders_fisheye::FisheyeFragOffAxis;
            case tuple(true, false, true, false):
                return shaders_fisheye::FisheyeFragOffAxisNormal;
            case tuple(true, false, false, true):
                return shaders_fisheye::FisheyeFragOffAxisPosition;
            case tuple(true, false, true, true):
                return shaders_fisheye::FisheyeFragOffAxisNormalPosition;

            case tuple(false, true, false, false):
                return shaders_fisheye::FisheyeFragDepth;
            case tuple(false, true, true, false):
                return shaders_fisheye::FisheyeFragDepthNormal;
            case tuple(false, true, false, true):
                return shaders_fisheye::FisheyeFragDepthPosition;
            case tuple(false, true, true, true):
                return shaders_fisheye::FisheyeFragDepthNormalPosition;

            case tuple(false, false, false, false):
                return shaders_fisheye::FisheyeFrag;
            case tuple(false, false, true, false):
                return shaders_fisheye::FisheyeFragNormal;
            case tuple(false, false, false, true):
                return shaders_fisheye::FisheyeFragPosition;
            case tuple(false, false, true, true):
                return shaders_fisheye::FisheyeFragNormalPosition;

            default:
                throw std::logic_error("Unhandled case label");
        }
    }(
        _isOffAxis,
        Engine::instance().settings().useDepthTexture,
        Engine::instance().settings().useNormalTexture,
        Engine::instance().settings().usePositionTexture
    );

    const std::string_view samplerShader =
        _isOffAxis ? shaders_fisheye::SampleOffsetFun : shaders_fisheye::SampleFun;

    _shader = ShaderProgram("FisheyeShader");
    _shader.addVertexShader(shaders_fisheye::BaseVert);
    _shader.addFragmentShader(fragmentShader);
    _shader.addFragmentShader(samplerShader);
    _shader.addFragmentShader(
        isCubic ?
            shaders_fisheye::InterpolateCubicFun :
            shaders_fisheye::InterpolateLinearFun
    );
    if (_method == FisheyeMethod::FourFaceCube) {
        _shader.addFragmentShader(shaders_fisheye::RotationFourFaceCubeFun);
    }
    else {
        // five or six face
        _shader.addFragmentShader(shaders_fisheye::RotationFiveSixFaceCubeFun);
    }


    _shader.createAndLinkProgram();
    _shader.bind();


    glUniform4fv(glGetUniformLocation(_shader.id(), "bgColor"), 1, &_clearColor.x);
    if (isCubic) {
        glUniform1f(
            glGetUniformLocation(_shader.id(), "size"),
            static_cast<float>(_cubemapResolution.x)
        );
    }

    _shaderLoc.cubemap = glGetUniformLocation(_shader.id(), "cubemap");
    glUniform1i(_shaderLoc.cubemap, 0);

    if (Engine::instance().settings().useDepthTexture) {
        _shaderLoc.depthCubemap = glGetUniformLocation(_shader.id(), "depthmap");
        glUniform1i(_shaderLoc.depthCubemap, 1);
    }

    if (Engine::instance().settings().useNormalTexture) {
        _shaderLoc.normalCubemap = glGetUniformLocation(_shader.id(), "normalmap");
        glUniform1i(_shaderLoc.normalCubemap, 2);
    }

    if (Engine::instance().settings().usePositionTexture) {
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

    if (Engine::instance().settings().useDepthTexture) {
        _depthCorrectionShader = ShaderProgram("FisheyeDepthCorrectionShader");
        _depthCorrectionShader.addVertexShader(shaders_fisheye::BaseVert);
        _depthCorrectionShader.addFragmentShader(
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

} // namespace sgct
