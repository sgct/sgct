/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/utils/plane.h>

#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/ogl_headers.h>
#include <sgct/helpers/vertexdata.h>
#include <array>

namespace sgct::utils {

Plane::Plane(float width, float height) {
    createVBO(width, height);

    if (!Engine::checkForOGLErrors()) {
        MessageHandler::instance()->printError("SGCT Utils: Plane creation error");
    }
}

Plane::~Plane() {
    glDeleteBuffers(1, &_vbo);
    glDeleteVertexArrays(1, &_vao);
}

void Plane::draw() {
    drawVAO();
}

void Plane::drawVBO() {
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    
    glInterleavedArrays(GL_T2F_N3F_V3F, 0, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glPopClientAttrib();
}

void Plane::drawVAO() {
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
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    MessageHandler::instance()->printDebug("Plane: Generating VAO: %d", _vao);
    glGenBuffers(1, &_vbo);

    MessageHandler::instance()->printDebug("Plane: Generating VBO: %d", _vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        4 * sizeof(helpers::VertexData),
        verts.data(),
        GL_STATIC_DRAW
    );

    // texcoords
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(helpers::VertexData),
        reinterpret_cast<void*>(0)
    );
    // normals
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(helpers::VertexData),
        reinterpret_cast<void*>(8)
    );
    // vert positions
    glVertexAttribPointer(
        2,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(helpers::VertexData),
        reinterpret_cast<void*>(20)
    );

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace sgct::utils
