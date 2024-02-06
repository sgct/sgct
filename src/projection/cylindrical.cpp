/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection/cylindrical.h>

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/fmt.h>
#include <sgct/internalshaders.h>
#include <sgct/log.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <sgct/settings.h>
#include <sgct/window.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    constexpr std::string_view FragmentShader = R"(
  #version 330 core

  in vec2 tr_uv;
  out vec4 out_diffuse;

  uniform samplerCube cubemap;
  uniform float rotation;
  uniform float heightOffset;

  const float PI = 3.141592654;

  void main() {
    vec2 pixelNormalized = tr_uv;
    float angle = 2.0 * PI * pixelNormalized.x;
    vec2 direction = vec2(cos(-angle + rotation), sin(-angle + rotation));

    vec3 sample = (vec3(direction, pixelNormalized.y + heightOffset));
    out_diffuse = texture(cubemap, sample);
  }
)";

    struct Vertex {
        float x;
        float y;
        float z;
        float s;
        float t;
    };
} // namespace

namespace sgct {

CylindricalProjection::CylindricalProjection(const Window* parent)
    : NonLinearProjection(parent)
{
    setUseDepthTransformation(true);
}

CylindricalProjection::~CylindricalProjection() {
    glDeleteBuffers(1, &_vbo);
    glDeleteVertexArrays(1, &_vao);
    _shader.deleteProgram();
}

void CylindricalProjection::render(const Window& window, const BaseViewport& viewport,
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

    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    glUniform1i(_shaderLoc.cubemap, 0);
    glUniform1f(_shaderLoc.rotation, glm::radians(_rotation));
    glUniform1f(_shaderLoc.heightOffset, _heightOffset);


    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    ShaderProgram::unbind();

    glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void CylindricalProjection::renderCubemap(Window& window, Frustum::Mode frustumMode) {
    ZoneScoped;

    renderCubeFaces(window, frustumMode);
}

void CylindricalProjection::update(vec2) {
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    constexpr std::array<float, 20> v = {
        -1.f, -1.f, -1.f, 0.f, 0.f,
        -1.f,  1.f, -1.f, 0.f, 1.f,
         1.f, -1.f, -1.f, 1.f, 0.f,
         1.f,  1.f, -1.f, 1.f, 1.f
    };
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void CylindricalProjection::initVBO() {
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

void CylindricalProjection::initViewports() {
    // radius is needed to calculate the distance to all view planes
    // const float radius = _diameter / 2.f;
    const float radius = _radius;

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
        _subViewports.right.setSize(vec2{ 1.f, 1.f });

        const glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(-90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );

        glm::vec4 upperRight = upperRightBase;
        upperRight.x = radius;

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
        _subViewports.left.setPos(vec2{ 0.f, 0.f });
        _subViewports.left.setSize(vec2{ 1.f, 1.f });

        const glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );

        glm::vec4 lowerLeft = lowerLeftBase;
        lowerLeft.x = -radius;
        glm::vec4 upperLeft = upperLeftBase;
        upperLeft.x = -radius;

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
        _subViewports.bottom.setPos(vec2{ 0.f, 0.f });
        _subViewports.bottom.setSize(vec2{ 1.f, 1.f });

        const glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(-90.f),
            glm::vec3(1.f, 0.f, 0.f)
        );

        glm::vec4 lowerLeft = lowerLeftBase;
        lowerLeft.y = -radius;

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
        _subViewports.top.setSize(vec2{ 1.f, 1.f });

        const glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(90.f),
            glm::vec3(1.f, 0.f, 0.f)
        );

        glm::vec4 upperLeft = upperLeftBase;
        upperLeft.y = radius;
        glm::vec4 upperRight = upperRightBase;
        upperRight.y = radius;

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

   _subViewports.back.setEnabled(false);
}

void CylindricalProjection::initShaders() {
    // reload shader program if it exists
    _shader.deleteProgram();

    _shader = ShaderProgram("CylindricalProjectionShader");
    _shader.addShaderSource(shaders_fisheye::BaseVert, GL_VERTEX_SHADER);
    _shader.addShaderSource(FragmentShader, GL_FRAGMENT_SHADER);
    _shader.createAndLinkProgram();
    _shader.bind();

    _shaderLoc.cubemap = glGetUniformLocation(_shader.id(), "cubemap");
    glUniform1i(_shaderLoc.cubemap, 0);
    _shaderLoc.rotation = glGetUniformLocation(_shader.id(), "rotation");
    _shaderLoc.heightOffset = glGetUniformLocation(_shader.id(), "heightOffset");

    ShaderProgram::unbind();
}

void CylindricalProjection::setRotation(float rotation) {
    _rotation = rotation;
}

void CylindricalProjection::setHeightOffset(float heightOffset) {
    _heightOffset = heightOffset;
}

void CylindricalProjection::setRadius(float radius) {
    _radius = radius;
}

} // namespace sgct
