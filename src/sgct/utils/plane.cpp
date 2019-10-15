/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/utils/plane.h>

#include <sgct/messagehandler.h>
#include <sgct/ogl_headers.h>
#include <sgct/helpers/vertexdata.h>
#include <array>

namespace sgct::utils {

Plane::Plane(float width, float height) {
    createVBO(width, height);
}

Plane::~Plane() {
    glDeleteBuffers(1, &_vbo);
    glDeleteVertexArrays(1, &_vao);
}

void Plane::draw() {
    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void Plane::createVBO(float width, float height) {
    std::array<helpers::VertexData, 4> verts;
    verts[0] = { 0.f, 0.f, 0.f, 0.f, 1.f, -width / 2.f, -height / 2.f, 0.f };
    verts[1] = { 1.f, 0.f, 0.f, 0.f, 1.f,  width / 2.f, -height / 2.f, 0.f };
    verts[2] = { 0.f, 1.f, 0.f, 0.f, 1.f, -width / 2.f,  height / 2.f, 0.f };
    verts[3] = { 1.f, 1.f, 0.f, 0.f, 1.f,  width / 2.f,  height / 2.f, 0.f };

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        4 * sizeof(helpers::VertexData),
        verts.data(),
        GL_STATIC_DRAW
    );

    // texcoords
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(helpers::VertexData),
        reinterpret_cast<void*>(0)
    );

    // normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(helpers::VertexData),
        reinterpret_cast<void*>(8)
    );

    // vert positions
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(helpers::VertexData),
        reinterpret_cast<void*>(20)
    );

    glBindVertexArray(0);
}

} // namespace sgct::utils
