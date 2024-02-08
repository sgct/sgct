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
#include <sgct/settings.h>
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

FisheyeProjection::FisheyeProjection(const Window* parent)
    : NonLinearProjection(parent)
{}

FisheyeProjection::~FisheyeProjection() {
    glDeleteBuffers(1, &_vbo);
    glDeleteVertexArrays(1, &_vao);
    _shader.deleteProgram();
    _depthCorrectionShader.deleteProgram();
}

void FisheyeProjection::update(vec2 size) {
    // do the cropping in the fragment shader and not by changing the vbo

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

    const std::array<const Vertex, 4> v = {
        Vertex{ -x, -y, -1.f, _cropLeft, _cropBottom },
        Vertex{ -x,  y, -1.f, _cropLeft, 1.f - _cropTop },
        Vertex{  x, -y, -1.f, 1.f - _cropRight, _cropBottom },
        Vertex{  x,  y, -1.f, 1.f - _cropRight, 1.f - _cropTop }
    };
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(Vertex), v.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void FisheyeProjection::render(const Window& window, const BaseViewport& viewport,
                               Frustum::Mode frustumMode)
{
    ZoneScoped;

    glEnable(GL_SCISSOR_TEST);
    Engine::instance().setupViewport(window, viewport, frustumMode);
    glClearColor(_clearColor.x, _clearColor.y, _clearColor.z, _clearColor.w);
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
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    glUniform1i(_shaderLoc.cubemap, 0);
    glUniform1f(_shaderLoc.halfFov, glm::radians<float>(_fov / 2.f));

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

void FisheyeProjection::renderCubemap(Window& window, Frustum::Mode frustumMode) {
    ZoneScoped;

    switch (frustumMode) {
        case Frustum::Mode::MonoEye:
            break;
        case Frustum::Mode::StereoLeftEye:
            setOffset(
                vec3{ -Engine::defaultUser().eyeSeparation() / _diameter, 0.f, 0.f }
            );
            break;
        case Frustum::Mode::StereoRightEye:
            setOffset(
                vec3{ Engine::defaultUser().eyeSeparation() / _diameter, 0.f, 0.f }
            );
            break;
    }

    auto render = [this](const Window& win, BaseViewport& vp, int idx, Frustum::Mode mode)
    {
        if (!vp.isEnabled()) {
            return;
        }

        renderCubeFace(win, vp, idx, mode);


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

            win.renderScreenQuad();
            ShaderProgram::unbind();

            glDisable(GL_DEPTH_TEST);
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

void FisheyeProjection::setOffset(vec3 offset) {
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
            _subViewports.right.setSize(vec2{ 1.f - cropLevel, 1.f });

            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(-90.f),
                glm::vec3(0.f, 1.f, 0.f)
            );

            glm::vec4 upperRight = upperRightBase;
            upperRight.x = projectionOffset;

            glm::vec3 ll = glm::vec3(rotMat * lowerLeftBase);
            glm::vec3 ul = glm::vec3(rotMat * upperLeftBase);
            glm::vec3 ur = glm::vec3(rotMat * upperRight);
            _subViewports.right.projectionPlane().setCoordinates(
                vec3(ll.x, ll.y, ll.z),
                vec3(ul.x, ul.y, ul.z),
                vec3(ur.x, ur.y, ur.z)
            );
        }

        // -X face
        {
            _subViewports.left.setPos(vec2{ cropLevel, 0.f });
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

            glm::vec3 ll = glm::vec3(rotMat * lowerLeft);
            glm::vec3 ul = glm::vec3(rotMat * upperLeft);
            glm::vec3 ur = glm::vec3(rotMat * upperRightBase);
            _subViewports.left.projectionPlane().setCoordinates(
                vec3(ll.x, ll.y, ll.z),
                vec3(ul.x, ul.y, ul.z),
                vec3(ur.x, ur.y, ur.z)
            );
        }

        // +Y face
        {
            _subViewports.bottom.setPos(vec2{ 0.f, cropLevel });
            _subViewports.bottom.setSize(vec2{ 1.f, 1.f - cropLevel });

            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(-90.f),
                glm::vec3(1.f, 0.f, 0.f)
            );

            glm::vec4 lowerLeft = lowerLeftBase;
            lowerLeft.y = -projectionOffset;

            glm::vec3 ll = glm::vec3(rotMat * lowerLeft);
            glm::vec3 ul = glm::vec3(rotMat * upperLeftBase);
            glm::vec3 ur = glm::vec3(rotMat * upperRightBase);
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

            glm::vec3 ll = glm::vec3(rotMat * lowerLeftBase);
            glm::vec3 ul = glm::vec3(rotMat * upperLeft);
            glm::vec3 ur = glm::vec3(rotMat * upperRight);
            _subViewports.top.projectionPlane().setCoordinates(
                vec3(ll.x, ll.y, ll.z),
                vec3(ul.x, ul.y, ul.z),
                vec3(ur.x, ur.y, ur.z)
            );
        }

        // +Z face
        {
            glm::vec3 ll = glm::vec3(rollRot * lowerLeftBase);
            glm::vec3 ul = glm::vec3(rollRot * upperLeftBase);
            glm::vec3 ur = glm::vec3(rollRot * upperRightBase);
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

                glm::vec3 ll = glm::vec3(rotMat * lowerLeftBase);
                glm::vec3 ul = glm::vec3(rotMat * upperLeftBase);
                glm::vec3 ur = glm::vec3(rotMat * upperRightBase);
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

            glm::vec3 ll = glm::vec3(rotMat * lowerLeftBase);
            glm::vec3 ul = glm::vec3(rotMat * upperLeftBase);
            glm::vec3 ur = glm::vec3(rotMat * upperRightBase);
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

            glm::vec3 ll = glm::vec3(rotMat * lowerLeftBase);
            glm::vec3 ul = glm::vec3(rotMat * upperLeftBase);
            glm::vec3 ur = glm::vec3(rotMat * upperRightBase);
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

            glm::vec3 ll = glm::vec3(rotMat * lowerLeftBase);
            glm::vec3 ul = glm::vec3(rotMat * upperLeftBase);
            glm::vec3 ur = glm::vec3(rotMat * upperRightBase);
            _subViewports.top.projectionPlane().setCoordinates(
                vec3(ll.x, ll.y, ll.z),
                vec3(ul.x, ul.y, ul.z),
                vec3(ur.x, ur.y, ur.z)
            );
        }

        // +Z face
        {
            glm::vec3 ll = glm::vec3(panRot * lowerLeftBase);
            glm::vec3 ul = glm::vec3(panRot * upperLeftBase);
            glm::vec3 ur = glm::vec3(panRot * upperRightBase);
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
    if (_isStereo || _preferedMonoFrustumMode != Frustum::Mode::MonoEye) {
        // if any frustum mode other than Mono (or stereo)
        _isOffAxis = true;
    }

    // reload shader program if it exists
    _shader.deleteProgram();

    const bool isCubic = (_interpolationMode == InterpolationMode::Cubic);
    std::string_view fragmentShader = [](bool isOffAxis, bool useDepth,
                                         Settings::DrawBufferType type)
    {
        // It would be nice to do a multidimensional switch statement -.-

        constexpr auto tuple = [](bool offAxis, bool depth,
                                  Settings::DrawBufferType t) -> uint16_t
        {
            // Injective mapping from <bool, bool, bool, DrawBufferType> to uint16_t
            uint16_t res = 0;
            res += static_cast<uint8_t>(t);
            if (offAxis) {
                res += 1 << 11;
            }
            if (depth) {
                res += 1 << 12;
            }
            return res;
        };

        using DrawBufferType = Settings::DrawBufferType;
        switch (tuple(isOffAxis, useDepth, type)) {
            case tuple(true, true, DrawBufferType::Diffuse):
                return shaders_fisheye::FisheyeFragOffAxisDepth;
            case tuple(true, true, DrawBufferType::DiffuseNormal):
                return shaders_fisheye::FisheyeFragOffAxisDepthNormal;
            case tuple(true, true, DrawBufferType::DiffusePosition):
                return shaders_fisheye::FisheyeFragOffAxisDepthPosition;
            case tuple(true, true, DrawBufferType::DiffuseNormalPosition):
                return shaders_fisheye::FisheyeFragOffAxisDepthNormalPosition;

            case tuple(true, false, DrawBufferType::Diffuse):
                return shaders_fisheye::FisheyeFragOffAxis;
            case tuple(true, false, DrawBufferType::DiffuseNormal):
                return shaders_fisheye::FisheyeFragOffAxisNormal;
            case tuple(true, false, DrawBufferType::DiffusePosition):
                return shaders_fisheye::FisheyeFragOffAxisPosition;
            case tuple(true, false, DrawBufferType::DiffuseNormalPosition):
                return shaders_fisheye::FisheyeFragOffAxisNormalPosition;

            case tuple(false, true, DrawBufferType::Diffuse):
                return shaders_fisheye::FisheyeFragDepth;
            case tuple(false, true, DrawBufferType::DiffuseNormal):
                return shaders_fisheye::FisheyeFragDepthNormal;
            case tuple(false, true, DrawBufferType::DiffusePosition):
                return shaders_fisheye::FisheyeFragDepthPosition;
            case tuple(false, true, DrawBufferType::DiffuseNormalPosition):
                return shaders_fisheye::FisheyeFragDepthNormalPosition;

            case tuple(false, false, DrawBufferType::Diffuse):
                return shaders_fisheye::FisheyeFrag;
            case tuple(false, false, DrawBufferType::DiffuseNormal):
                return shaders_fisheye::FisheyeFragNormal;
            case tuple(false, false, DrawBufferType::DiffusePosition):
                return shaders_fisheye::FisheyeFragPosition;
            case tuple(false, false, DrawBufferType::DiffuseNormalPosition):
                return shaders_fisheye::FisheyeFragNormalPosition;

            default:
                throw std::logic_error("Unhandled case label");
        }
    }(
        _isOffAxis,
        Settings::instance().useDepthTexture(),
        Settings::instance().drawBufferType()
    );

    std::string_view samplerShader =
        _isOffAxis ? shaders_fisheye::SampleOffsetFun : shaders_fisheye::SampleFun;

    _shader = ShaderProgram("FisheyeShader");
    _shader.addShaderSource(shaders_fisheye::BaseVert, GL_VERTEX_SHADER);
    _shader.addShaderSource(fragmentShader, GL_FRAGMENT_SHADER);
    _shader.addShaderSource(samplerShader, GL_FRAGMENT_SHADER);
    _shader.addShaderSource(
        isCubic ?
            shaders_fisheye::InterpolateCubicFun :
            shaders_fisheye::InterpolateLinearFun,
        GL_FRAGMENT_SHADER
    );
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


    glUniform4fv(glGetUniformLocation(_shader.id(), "bgColor"), 1, &_clearColor.x);
    if (isCubic) {
        glUniform1f(
            glGetUniformLocation(_shader.id(), "size"),
            static_cast<float>(_cubemapResolution.x)
        );
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
            GL_VERTEX_SHADER
        );
        _depthCorrectionShader.addShaderSource(
            shaders_fisheye::FisheyeDepthCorrectionFrag,
            GL_FRAGMENT_SHADER
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
