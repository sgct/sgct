/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection/equirectangular.h>

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/internalshaders.h>
#include <sgct/log.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/profiling.h>
#include <sgct/settings.h>
#include <sgct/window.h>

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

EquirectangularProjection::EquirectangularProjection(const Window* parent)
    : NonLinearProjection(parent)
{
    setUseDepthTransformation(true);
}

EquirectangularProjection::~EquirectangularProjection() {
    glDeleteBuffers(1, &_vbo);
    glDeleteVertexArrays(1, &_vao);
    _shader.deleteProgram();
}

void EquirectangularProjection::render(const Window& window, const BaseViewport& viewport,
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

void EquirectangularProjection::renderCubemap(Window& window, Frustum::Mode frustumMode) {
    ZoneScoped

    renderCubeFaces(window, frustumMode);
}

void EquirectangularProjection::update(glm::vec2) {
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    const std::array<const float, 20> v = {
        -1.f, -1.f, -1.f, 0.f, 0.f,
        -1.f,  1.f, -1.f, 0.f, 1.f,
         1.f, -1.f, -1.f, 1.f, 0.f,
         1.f,  1.f, -1.f, 1.f, 1.f
    };
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void EquirectangularProjection::initVBO() {
    glGenVertexArrays(1, &_vao);
    Log::Debug("Generating VAO: %d", _vao);
    glBindVertexArray(_vao);

    glGenBuffers(1, &_vbo);
    Log::Debug("Generating VBO: %d", _vbo);
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

void EquirectangularProjection::initViewports() {
    // radius is needed to calculate the distance to all view planes
    // const float radius = _diameter / 2.f;
    const float radius = 1.f;

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
        glm::mat4 r = glm::rotate(rollRot, glm::radians(180.f), glm::vec3(0.f, 1.f, 0.f));
        _subViewports.back.projectionPlane().setCoordinates(
            glm::vec3(r * lowerLeftBase),
            glm::vec3(r * upperLeftBase),
            glm::vec3(r * upperRightBase)
        );
    }
}

void EquirectangularProjection::initShaders() {
    // reload shader program if it exists
    _shader.deleteProgram();

    std::string fragmentShader = R"(
  #version 330 core

  in vec2 tr_uv;
  out vec4 out_diffuse;

  uniform samplerCube cubemap;

  const float PI = 3.141592654;

  void main() {
    float phi = PI * (1.0 - tr_uv.t);
    float theta = 2.0 * PI * (tr_uv.s - 0.5);
    float x = sin(phi) * sin(theta);
    float y = sin(phi) * cos(theta);
    float z = cos(phi);
    out_diffuse = texture(cubemap, vec3(x, y, z));
  }
)";

    _shader = ShaderProgram("CylindricalProjectinoShader");
    _shader.addShaderSource(shaders_fisheye::BaseVert, fragmentShader);
    _shader.createAndLinkProgram();
    _shader.bind();

    _shaderLoc.cubemap = glGetUniformLocation(_shader.id(), "cubemap");
    glUniform1i(_shaderLoc.cubemap, 0);

    ShaderProgram::unbind();
}

} // namespace sgct
