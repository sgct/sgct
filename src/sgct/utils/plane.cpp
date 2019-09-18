/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/utils/plane.h>

#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/ogl_headers.h>
#include <sgct/helpers/vertexdata.h>
#include <array>

namespace sgct_utils {

Plane::Plane(float width, float height) {
    createVBO(width, height);

    if (!sgct::Engine::checkForOGLErrors()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error, "SGCT Utils: Plane creation error\n"
        );
    }
}

Plane::~Plane() {
    glDeleteBuffers(1, &mVBO);
    mVBO = 0;

    glDeleteVertexArrays(1, &mVAO);
    mVAO = 0;
}

void Plane::draw() {
    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        drawVBO();
    }
    else {
        drawVAO();
    }
}

void Plane::drawVBO() {
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    
    glInterleavedArrays(GL_T2F_N3F_V3F, 0, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glPopClientAttrib();
}

void Plane::drawVAO() {
    glBindVertexArray(mVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void Plane::createVBO(float width, float height) {
    std::array<sgct_helpers::VertexData, 4> verts;
    verts[0] = { 0.f, 0.f, 0.f, 0.f, 1.f, -width / 2.f, -height / 2.f, 0.f };
    verts[1] = { 1.f, 0.f, 0.f, 0.f, 1.f,  width / 2.f, -height / 2.f, 0.f };
    verts[2] = { 0.f, 1.f, 0.f, 0.f, 1.f, -width / 2.f,  height / 2.f, 0.f };
    verts[3] = { 1.f, 1.f, 0.f, 0.f, 1.f,  width / 2.f,  height / 2.f, 0.f };


    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glGenVertexArrays(1, &mVAO);
        glBindVertexArray(mVAO);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug, "Plane: Generating VAO: %d\n", mVAO
        );
    }
    
    glGenBuffers(1, &mVBO);
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug, "Plane: Generating VBO: %d\n", mVBO
    );

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        4 * sizeof(sgct_helpers::VertexData),
        verts.data(),
        GL_STATIC_DRAW
    );

    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        // texcoords
        glVertexAttribPointer(
            0,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(sgct_helpers::VertexData),
            reinterpret_cast<void*>(0)
        );
        // normals
        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(sgct_helpers::VertexData),
            reinterpret_cast<void*>(8)
        );
        // vert positions
        glVertexAttribPointer(
            2,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(sgct_helpers::VertexData),
            reinterpret_cast<void*>(20)
        );
    }

    // unbind
    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glBindVertexArray(0);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace sgct_utils
