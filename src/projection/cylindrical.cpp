/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection/cylindrical.h>

#include <sgct/baseviewport.h>
#include <sgct/config.h>
#include <sgct/definitions.h>
#include <sgct/projection/projectionplane.h>
#include <sgct/internalshaders.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <sgct/window.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <cstddef>
#include <string_view>

namespace {
    constexpr std::string_view FragmentShader = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
  } in_data;

  out vec4 out_diffuse;

  uniform samplerCube cubemap;
  uniform float rotation;
  uniform float heightOffset;

  const float PI = 3.141592654;


  void main() {
    vec2 pixelNormalized = in_data.texCoords;
    float angle = 2.0 * PI * pixelNormalized.x;
    vec2 direction = vec2(cos(-angle + rotation), sin(-angle + rotation));

    vec3 tex = vec3(direction, pixelNormalized.y + heightOffset);
    out_diffuse = texture(cubemap, tex);
  }
)";
} // namespace

namespace sgct {

CylindricalProjection::CylindricalProjection(const config::CylindricalProjection& config,
                                             const Window& parent, User& user)
    : NonLinearProjection(parent)
    , _rotation(config.rotation.value_or(0.f))
    , _heightOffset(config.heightOffset.value_or(0.f))
    , _radius(config.radius.value_or(5.f))
{
    setUser(user);
    setUseDepthTransformation(true);

    if (config.quality) {
        setCubemapResolution(*config.quality);
    }
}

CylindricalProjection::~CylindricalProjection() {
    glDeleteBuffers(1, &_vbo);
    glDeleteVertexArrays(1, &_vao);
    _shader.program.deleteProgram();
}

void CylindricalProjection::render(const BaseViewport& viewport,
                                   FrustumMode frustumMode) const
{
    ZoneScoped;

    glEnable(GL_SCISSOR_TEST);
    viewport.setupViewport(frustumMode);
    glClearColor(_clearColor.x, _clearColor.y, _clearColor.z, _clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    glBindTextureUnit(0, _textures.cubeMapColor);

    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    glProgramUniform1i(_shader.program.id(), _shader.cubemap, 0);
    glProgramUniform1f(_shader.program.id(), _shader.rotation, glm::radians(_rotation));
    glProgramUniform1f(_shader.program.id(), _shader.heightOffset, _heightOffset);

    _shader.program.bind();
    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    ShaderProgram::unbind();

    glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void CylindricalProjection::renderCubemap(FrustumMode frustumMode) const {
    ZoneScoped;

    renderCubeFace(_subViewports.right, 0, frustumMode);
    renderCubeFace(_subViewports.left, 1, frustumMode);
    renderCubeFace(_subViewports.bottom, 2, frustumMode);
    renderCubeFace(_subViewports.top, 3, frustumMode);
    renderCubeFace(_subViewports.front, 4, frustumMode);
    renderCubeFace(_subViewports.back, 5, frustumMode);
}

void CylindricalProjection::update(const vec2&) const {}

void CylindricalProjection::initVBO() {
    struct Vertex {
        float x;
        float y;
        float z;
        float s;
        float t;
    };

    glCreateBuffers(1, &_vbo);
    constexpr std::array<Vertex, 4> v = {
        Vertex{ -1.f, -1.f, -1.f, 0.f, 0.f },
        Vertex{ -1.f,  1.f, -1.f, 0.f, 1.f },
        Vertex{  1.f, -1.f, -1.f, 1.f, 0.f },
        Vertex{  1.f,  1.f, -1.f, 1.f, 1.f }
    };
    glNamedBufferStorage(_vbo, 4 * sizeof(Vertex), v.data(), GL_NONE_BIT);

    glCreateVertexArrays(1, &_vao);
    glVertexArrayVertexBuffer(_vao, 0, _vbo, 0, sizeof(Vertex));

    glEnableVertexArrayAttrib(_vao, 0);
    glVertexArrayAttribFormat(_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(_vao, 0, 0);

    glEnableVertexArrayAttrib(_vao, 1);
    glVertexArrayAttribFormat(_vao, 1, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, s));
    glVertexArrayAttribBinding(_vao, 1, 0);
}

void CylindricalProjection::initViewports() {
    // setup base viewport that will be rotated to create the other cubemap views
    // +Z face
    const glm::vec4 lowerLeftBase(-_radius, -_radius, _radius, 1.f);
    const glm::vec4 upperLeftBase(-_radius, _radius, _radius, 1.f);
    const glm::vec4 upperRightBase(_radius, _radius, _radius, 1.f);

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
        const glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(-90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );

        glm::vec4 upperRight = upperRightBase;
        upperRight.x = _radius;

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
        const glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );

        glm::vec4 lowerLeft = lowerLeftBase;
        lowerLeft.x = -_radius;
        glm::vec4 upperLeft = upperLeftBase;
        upperLeft.x = -_radius;

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
        const glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(-90.f),
            glm::vec3(1.f, 0.f, 0.f)
        );

        glm::vec4 lowerLeft = lowerLeftBase;
        lowerLeft.y = -_radius;

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
        const glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(90.f),
            glm::vec3(1.f, 0.f, 0.f)
        );

        glm::vec4 upperLeft = upperLeftBase;
        upperLeft.y = _radius;
        glm::vec4 upperRight = upperRightBase;
        upperRight.y = _radius;

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

   _subViewports.back.setEnabled(false);
}

void CylindricalProjection::initShaders() {
    _shader.program = ShaderProgram("CylindricalProjectionShader");
    _shader.program.addVertexShader(shaders_fisheye::BaseVert);
    _shader.program.addFragmentShader(FragmentShader);
    _shader.program.createAndLinkProgram();

    _shader.cubemap = glGetUniformLocation(_shader.program.id(), "cubemap");
    glProgramUniform1i(_shader.program.id(), _shader.cubemap, 0);
    _shader.rotation = glGetUniformLocation(_shader.program.id(), "rotation");
    _shader.heightOffset = glGetUniformLocation(_shader.program.id(), "heightOffset");
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
