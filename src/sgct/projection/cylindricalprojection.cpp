/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection/cylindricalprojection.h>

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/profiling.h>
#include <sgct/settings.h>
#include <sgct/window.h>
#include <sgct/helpers/stringfunctions.h>
#include <sgct/shaders/internalfisheyeshaders.h>
#include <sgct/shaders/internalfisheyeshaders_cubic.h>

namespace sgct {

CylindricalProjection::CylindricalProjection(const Window* parent)
    : NonLinearProjection(parent)
{
    setUseDepthTransformation(true);
}

void CylindricalProjection::render(const Window& window, const BaseViewport& viewport,
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
    glm::ivec2 size = window.framebufferResolution();
    glUniform2f(_shaderLoc.size, static_cast<float>(size.x), static_cast<float>(size.y));


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

void CylindricalProjection::renderCubemap(Window& window, Frustum::Mode frustumMode) {
    ZoneScoped

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
    };

    render(window, _subViewports.right, 0, frustumMode);
    render(window, _subViewports.left, 1, frustumMode);
    render(window, _subViewports.bottom, 2, frustumMode);
    render(window, _subViewports.top, 3, frustumMode);
    render(window, _subViewports.front, 4, frustumMode);
    render(window, _subViewports.back, 5, frustumMode);
}

void CylindricalProjection::update(glm::vec2) {
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    const std::array<const float, 20> v = {
        0.f, 0.f, -1.f, -1.f, -1.f,
        0.f, 1.f, -1.f,  1.f, -1.f,
        1.f, 0.f,  1.f, -1.f, -1.f,
        1.f, 1.f,  1.f,  1.f, -1.f
    };
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void CylindricalProjection::initViewports() {
    // radius is needed to calculate the distance to all view planes
    // const float radius = _diameter / 2.f;
    const float radius = 5.f;

    // setup base viewport that will be rotated to create the other cubemap views
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
        _subViewports.right.setSize(glm::vec2(1.f, 1.f));

        const glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(-90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );

        glm::vec4 upperRight = upperRightBase;
        upperRight.x = radius;

        _subViewports.right.projectionPlane().setCoordinates(
            glm::vec3(rotMat * lowerLeftBase),
            glm::vec3(rotMat * upperLeftBase),
            glm::vec3(rotMat * upperRight)
        );
    }

    // -X face
    {
        _subViewports.left.setPos(glm::vec2(0.f, 0.f));
        _subViewports.left.setSize(glm::vec2(1.f, 1.f));

        const glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );

        glm::vec4 lowerLeft = lowerLeftBase;
        lowerLeft.x = -radius;
        glm::vec4 upperLeft = upperLeftBase;
        upperLeft.x = -radius;

        _subViewports.left.projectionPlane().setCoordinates(
            glm::vec3(rotMat * lowerLeft),
            glm::vec3(rotMat * upperLeft),
            glm::vec3(rotMat * upperRightBase)
        );
    }

    // +Y face
    {
        _subViewports.bottom.setPos(glm::vec2(0.f, 0.f));
        _subViewports.bottom.setSize(glm::vec2(1.f, 1.f));

        const glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(-90.f),
            glm::vec3(1.f, 0.f, 0.f)
        );

        glm::vec4 lowerLeft = lowerLeftBase;
        lowerLeft.y = -radius;

        _subViewports.bottom.projectionPlane().setCoordinates(
            glm::vec3(rotMat * lowerLeft),
            glm::vec3(rotMat * upperLeftBase),
            glm::vec3(rotMat * upperRightBase)
        );
    }

    // -Y face
    {
        _subViewports.top.setSize(glm::vec2(1.f, 1.f));

        const glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(90.f),
            glm::vec3(1.f, 0.f, 0.f)
        );

        glm::vec4 upperLeft = upperLeftBase;
        upperLeft.y = radius;
        glm::vec4 upperRight = upperRightBase;
        upperRight.y = radius;

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
    // _subViewports.bottom.setEnabled(false);
    // _subViewports.top.setEnabled(false);
}

void CylindricalProjection::initShaders() {
    // reload shader program if it exists
    _shader.deleteProgram();

    const bool isCubic = (_interpolationMode == InterpolationMode::Cubic);

    std::string fragmentShader = R"(
  #version 330 core

  in vec2 tr_uv;
  out vec4 out_diffuse;

  uniform samplerCube cubemap;
  uniform vec2 size;

  // vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg) {

  //   float s = 2.0 * (texel.s - 0.5);
  //   float t = 2.0 * (texel.t - 0.5);
  //   float r2 = s*s + t*t;
  //   if (r2 <= 1.0) {
  //     float phi = sqrt(r2) * halfFov;
  //     float theta = atan(s, t);
  //     float x = sin(phi) * sin(theta);
  //     float y = -sin(phi) * cos(theta);
  //     float z = cos(phi);

  //     vec3 rotVec = vec3(1.0, 0.0, 0.0);
  //     return texture(map, rotVec);
  //   }
  //   else {
  //     return bg;
  //   }
  // }

  const float PI = 3.141592654;


  void main() {
    vec2 pixel = gl_FragCoord.xy;
    vec2 pixelNormalized = pixel / size;
    float angle = pixelNormalized.x * 2.0 * PI;
    vec2 direction = vec2(cos(angle), sin(angle));

    vec3 sample = (vec3(direction, pixelNormalized.y));
    out_diffuse = texture(cubemap, sample);

    out_diffuse = texture(cubemap, sample.xyz);

//    out_diffuse = vec4(pixelNormalized, 0.0, 1.0);
  }

)";


    // replace color
    std::string color = "vec4(" + std::to_string(_clearColor.r) + ',' +
        std::to_string(_clearColor.g) + ',' + std::to_string(_clearColor.b) + ',' +
        std::to_string(_clearColor.a) + ')';
    helpers::findAndReplace(fragmentShader, "**bgColor**", color);

    _shader = ShaderProgram("CylindricalProjectinoShader");
    _shader.addShaderSource(shaders_fisheye::FisheyeVert, fragmentShader);
    _shader.createAndLinkProgram();
    _shader.bind();

    _shaderLoc.cubemap = glGetUniformLocation(_shader.id(), "cubemap");
    glUniform1i(_shaderLoc.cubemap, 0);
    _shaderLoc.size = glGetUniformLocation(_shader.id(), "size");

    ShaderProgram::unbind();
}

void CylindricalProjection::drawCubeFace(BaseViewport& face, RenderData renderData) {
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

void CylindricalProjection::blitCubeFace(int face) {
    // copy AA-buffer to "regular"/non-AA buffer
    _cubeMapFbo->bindBlit();
    attachTextures(face);
    _cubeMapFbo->blit();
}

void CylindricalProjection::attachTextures(int face) {
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

void CylindricalProjection::setRotation(float rotation) {
    _rotation = rotation;
}

} // namespace sgct
