/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/utils/box.h>

#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/ogl_headers.h>
#include <sgct/helpers/vertexdata.h>

namespace sgct::utils {

Box::Box(float size, TextureMappingMode mode) {
    createVBO(size, mode);

    if (!Engine::checkForOGLErrors()) {
        MessageHandler::instance()->printError("SGCT Utils: Box creation error\n");
    }
}

Box::~Box() {
    glDeleteBuffers(1, &_vbo);
    glDeleteVertexArrays(1, &_vao);
}

void Box::draw() {
    drawVAO();
}

void Box::drawVBO() {
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    
    glInterleavedArrays(GL_T2F_N3F_V3F, 0, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glPopClientAttrib();
}

void Box::drawVAO() {
    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void Box::createVBO(float size, TextureMappingMode tmm) {
    std::vector<helpers::VertexData> v(36);

    if (tmm == TextureMappingMode::Regular) {
        // A (front/+z)
        v[0] = { 0.f, 1.f, 0.f, 0.f, 1.f, -size / 2.f,  size / 2.f, size / 2.f };
        v[1] = { 0.f, 0.f, 0.f, 0.f, 1.f, -size / 2.f, -size / 2.f, size / 2.f };
        v[2] = { 1.f, 0.f, 0.f, 0.f, 1.f,  size / 2.f, -size / 2.f, size / 2.f };
        v[3] = { 0.f, 1.f, 0.f, 0.f, 1.f, -size / 2.f,  size / 2.f, size / 2.f };
        v[4] = { 1.f, 0.f, 0.f, 0.f, 1.f,  size / 2.f, -size / 2.f, size / 2.f };
        v[5] = { 1.f, 1.f, 0.f, 0.f, 1.f,  size / 2.f,  size / 2.f, size / 2.f };

        // B (right/+x)
        v[6] = { 0.f, 1.f, 1.f, 0.f, 0.f, size / 2.f,  size / 2.f,  size / 2.f };
        v[7] = { 0.f, 0.f, 1.f, 0.f, 0.f, size / 2.f, -size / 2.f,  size / 2.f };
        v[8] = { 1.f, 0.f, 1.f, 0.f, 0.f, size / 2.f, -size / 2.f, -size / 2.f };
        v[9] = { 0.f, 1.f, 1.f, 0.f, 0.f, size / 2.f,  size / 2.f,  size / 2.f };
        v[10] = { 1.f, 0.f, 1.f, 0.f, 0.f, size / 2.f, -size / 2.f, -size / 2.f };
        v[11] = { 1.f, 1.f, 1.f, 0.f, 0.f, size / 2.f,  size / 2.f, -size / 2.f };

        // C (Back/-z)
        v[12] = { 0.f, 1.f, 0.f, 0.f, -1.f,  size / 2.f,  size / 2.f, -size / 2.f };
        v[13] = { 0.f, 0.f, 0.f, 0.f, -1.f,  size / 2.f, -size / 2.f, -size / 2.f };
        v[14] = { 1.f, 0.f, 0.f, 0.f, -1.f, -size / 2.f, -size / 2.f, -size / 2.f };
        v[15] = { 0.f, 1.f, 0.f, 0.f, -1.f,  size / 2.f,  size / 2.f, -size / 2.f };
        v[16] = { 1.f, 0.f, 0.f, 0.f, -1.f, -size / 2.f, -size / 2.f, -size / 2.f };
        v[17] = { 1.f, 1.f, 0.f, 0.f, -1.f, -size / 2.f,  size / 2.f, -size / 2.f };

        // D (Left/-x)
        v[18] = { 0.f, 1.f, -1.f, 0.f, 0.f, -size / 2.f,  size / 2.f, -size / 2.f };
        v[19] = { 0.f, 0.f, -1.f, 0.f, 0.f, -size / 2.f, -size / 2.f, -size / 2.f };
        v[20] = { 1.f, 0.f, -1.f, 0.f, 0.f, -size / 2.f, -size / 2.f,  size / 2.f };
        v[21] = { 0.f, 1.f, -1.f, 0.f, 0.f, -size / 2.f,  size / 2.f, -size / 2.f };
        v[22] = { 1.f, 0.f, -1.f, 0.f, 0.f, -size / 2.f, -size / 2.f,  size / 2.f };
        v[23] = { 1.f, 1.f, -1.f, 0.f, 0.f, -size / 2.f,  size / 2.f,  size / 2.f };

        // E (Top/+y)
        v[24] = { 0.f, 1.f, 0.f, 1.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        v[25] = { 0.f, 0.f, 0.f, 1.f, 0.f, -size / 2.f, size / 2.f,  size / 2.f };
        v[26] = { 1.f, 0.f, 0.f, 1.f, 0.f,  size / 2.f, size / 2.f,  size / 2.f };
        v[27] = { 0.f, 1.f, 0.f, 1.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        v[28] = { 1.f, 0.f, 0.f, 1.f, 0.f,  size / 2.f, size / 2.f,  size / 2.f };
        v[29] = { 1.f, 1.f, 0.f, 1.f, 0.f,  size / 2.f, size / 2.f, -size / 2.f };

        // F (Bottom/-y)
        v[30] = { 0.f, 1.f, 0.f, -1.f, 0.f, -size / 2.f, -size / 2.f,  size / 2.f };
        v[31] = { 0.f, 0.f, 0.f, -1.f, 0.f, -size / 2.f, -size / 2.f, -size / 2.f };
        v[32] = { 1.f, 0.f, 0.f, -1.f, 0.f,  size / 2.f, -size / 2.f, -size / 2.f };
        v[33] = { 0.f, 1.f, 0.f, -1.f, 0.f, -size / 2.f, -size / 2.f,  size / 2.f };
        v[34] = { 1.f, 0.f, 0.f, -1.f, 0.f,  size / 2.f, -size / 2.f, -size / 2.f };
        v[35] = { 1.f, 1.f, 0.f, -1.f, 0.f,  size / 2.f, -size / 2.f,  size / 2.f };
    }
    else if (tmm == TextureMappingMode::CubeMap) {
        // A (front/+z)
        v[0] = { 0.f, 1.f, 0.f, 0.f, 1.f, -size / 2.f, size / 2.f, size / 2.f };
        v[1] = { 0.f, 0.5f, 0.f, 0.f, 1.f, -size / 2.f, -size / 2.f, size / 2.f };
        v[2] = { 0.333333f, 0.5f, 0.f, 0.f, 1.f, size / 2.f, -size / 2.f, size / 2.f };
        v[3] = { 0.f, 1.f, 0.f, 0.f, 1.f, -size / 2.f, size / 2.f, size / 2.f };
        v[4] = { 0.333333f, 0.5f, 0.f, 0.f, 1.f, size / 2.f, -size / 2.f, size / 2.f };
        v[5] = { 0.333333f, 1.f, 0.f, 0.f, 1.f, size / 2.f, size / 2.f, size / 2.f };

        // B (right/+x)
        v[6] = { 0.333334f, 1.f, 1.f, 0.f, 0.f, size / 2.f, size / 2.f, size / 2.f };
        v[7] = { 0.333334f, 0.5f, 1.f, 0.f, 0.f, size / 2.f, -size / 2.f, size / 2.f };
        v[8] = { 0.666666f, 0.5f, 1.f, 0.f, 0.f, size / 2.f, -size / 2.f, -size / 2.f };
        v[9] = { 0.333334f, 1.f, 1.f, 0.f, 0.f, size / 2.f, size / 2.f, size / 2.f };
        v[10] = { 0.666666f, 0.5f, 1.f, 0.f, 0.f, size / 2.f, -size / 2.f, -size / 2.f };
        v[11] = { 0.666666f, 1.f, 1.f, 0.f, 0.f, size / 2.f, size / 2.f, -size / 2.f };

        // C (Back/-z)
        v[12] = { 0.666667f, 1.f, 0.f, 0.f, -1.f, size / 2.f, size / 2.f, -size / 2.f };
        v[13] = { 0.666667f, 0.5f, 0.f, 0.f, -1.f, size / 2.f, -size / 2.f, -size / 2.f };
        v[14] = { 1.f, 0.5f, 0.f, 0.f, -1.f, -size / 2.f, -size / 2.f, -size / 2.f };
        v[15] = { 0.666667f, 1.f, 0.f, 0.f, -1.f, size / 2.f, size / 2.f, -size / 2.f };
        v[16] = { 1.f, 0.5f, 0.f, 0.f, -1.f, -size / 2.f, -size / 2.f, -size / 2.f };
        v[17] = { 1.f, 1.f, 0.f, 0.f, -1.f, -size / 2.f, size / 2.f, -size / 2.f };

        // D (Left/-x)
        v[18] = { 0.f, 0.5f, -1.f, 0.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        v[19] = { 0.f, 0.f, -1.f, 0.f, 0.f, -size / 2.f, -size / 2.f, -size / 2.f };
        v[20] = { 0.333333f, 0.f, -1.f, 0.f, 0.f, -size / 2.f, -size / 2.f, size / 2.f };
        v[21] = { 0.f, 0.5f, -1.f, 0.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        v[22] = { 0.333333f, 0.f, -1.f, 0.f, 0.f, -size / 2.f, -size / 2.f, size / 2.f };
        v[23] = { 0.333333f, 0.5f, -1.f, 0.f, 0.f, -size / 2.f, size / 2.f, size / 2.f };

        // E (Top/+y)
        v[24] = { 0.333334f, 0.5f, 0.f, 1.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        v[25] = { 0.333334f, 0.f, 0.f, 1.f, 0.f, -size / 2.f, size / 2.f, size / 2.f };
        v[26] = { 0.666666f, 0.f, 0.f, 1.f, 0.f, size / 2.f, size / 2.f, size / 2.f };
        v[27] = { 0.333334f, 0.5f, 0.f, 1.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        v[28] = { 0.666666f, 0.f, 0.f, 1.f, 0.f, size / 2.f, size / 2.f, size / 2.f };
        v[29] = { 0.666666f, 0.5f, 0.f, 1.f, 0.f, size / 2.f, size / 2.f, -size / 2.f };

        // F (Bottom/-y)
        v[30] = { 0.666667f, 0.5f, 0.f, -1.f, 0.f, -size / 2.f, -size / 2.f, size / 2.f };
        v[31] = { 0.666667f, 0.f, 0.f, -1.f, 0.f, -size / 2.f, -size / 2.f, -size / 2.f };
        v[32] = { 1.f, 0.f, 0.f, -1.f, 0.f, size / 2.f, -size / 2.f, -size / 2.f };
        v[33] = { 0.666667f, 0.5f, 0.f, -1.f, 0.f, -size / 2.f, -size / 2.f, size / 2.f };
        v[34] = { 1.f, 0.f, 0.f, -1.f, 0.f, size / 2.f, -size / 2.f, -size / 2.f };
        v[35] = { 1.f, 0.5f, 0.f, -1.f, 0.f, size / 2.f, -size / 2.f, size / 2.f };
    }
    else { // skybox
        // A (front/+z)
        v[0] = { 1.f, 0.666666f, 0.f, 0.f, 1.f, -size / 2.f, size / 2.f, size / 2.f };
        v[1] = { 1.f, 0.333334f, 0.f, 0.f, 1.f, -size / 2.f, -size / 2.f, size / 2.f };
        v[2] = { 0.751f, 0.333334f, 0.f, 0.f, 1.f, size / 2.f, -size / 2.f, size / 2.f };
        v[3] = { 1.f, 0.666666f, 0.f, 0.f, 1.f, -size / 2.f, size / 2.f, size / 2.f };
        v[4] = { 0.751f, 0.333334f, 0.f, 0.f, 1.f, size / 2.f, -size / 2.f, size / 2.f };
        v[5] = { 0.751f, 0.666666f, 0.f, 0.f, 1.f, size / 2.f, size / 2.f, size / 2.f };

        // B (right/+x)
        v[6] = { 0.749f, 0.666666f, 1.f, 0.f, 0.f, size / 2.f, size / 2.f, size / 2.f };
        v[7] = { 0.749f, 0.333334f, 1.f, 0.f, 0.f, size / 2.f, -size / 2.f, size / 2.f };
        v[8] = { 0.501f, 0.333334f, 1.f, 0.f, 0.f, size / 2.f, -size / 2.f, -size / 2.f };
        v[9] = { 0.749f, 0.666666f, 1.f, 0.f, 0.f, size / 2.f, size / 2.f, size / 2.f };
        v[10] = { 0.501f, 0.333334f, 1.f, 0.f, 0.f, size / 2.f, -size / 2.f, -size / 2.f };
        v[11] = { 0.501f, 0.666666f, 1.f, 0.f, 0.f, size / 2.f, size / 2.f, -size / 2.f };

        // C (Back/-z)
        v[12] = { 0.499f, 0.666666f, 0.f, 0.f, -1.f,  size / 2.f,  size / 2.f, -size / 2.f };
        v[13] = { 0.499f, 0.333334f, 0.f, 0.f, -1.f,  size / 2.f, -size / 2.f, -size / 2.f };
        v[14] = { 0.251f, 0.333334f, 0.f, 0.f, -1.f, -size / 2.f, -size / 2.f, -size / 2.f };
        v[15] = { 0.499f, 0.666666f, 0.f, 0.f, -1.f,  size / 2.f,  size / 2.f, -size / 2.f };
        v[16] = { 0.251f, 0.333334f, 0.f, 0.f, -1.f, -size / 2.f, -size / 2.f, -size / 2.f };
        v[17] = { 0.251f, 0.666666f, 0.f, 0.f, -1.f, -size / 2.f,  size / 2.f, -size / 2.f };

        // D (Left/-x)
        v[18] = { 0.249f, 0.666666f, -1.f, 0.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        v[19] = { 0.249f, 0.333334f, -1.f, 0.f, 0.f, -size / 2.f, -size / 2.f, -size / 2.f };
        v[20] = { 0.000f, 0.333334f, -1.f, 0.f, 0.f, -size / 2.f, -size / 2.f, size / 2.f };
        v[21] = { 0.249f, 0.666666f, -1.f, 0.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        v[22] = { 0.000f, 0.333334f, -1.f, 0.f, 0.f, -size / 2.f, -size / 2.f, size / 2.f };
        v[23] = { 0.000f, 0.666666f, -1.f, 0.f, 0.f, -size / 2.f, size / 2.f, size / 2.f };

        // E (Top/+y)
        v[24] = { 0.251f, 0.666667f, 0.f, 1.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        v[25] = { 0.251f, 1.f, 0.f, 1.f, 0.f, -size / 2.f, size / 2.f,  size / 2.f };
        v[26] = { 0.499f, 1.f, 0.f, 1.f, 0.f,  size / 2.f, size / 2.f,  size / 2.f };
        v[27] = { 0.251f, 0.666667f, 0.f, 1.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        v[28] = { 0.499f, 1.f, 0.f, 1.f, 0.f,  size / 2.f, size / 2.f,  size / 2.f };
        v[29] = { 0.499f, 0.666667f, 0.f, 1.f, 0.f,  size / 2.f, size / 2.f, -size / 2.f };

        // F (Bottom/-y)
        v[30] = { 0.251f, 0.f, 0.f, -1.f, 0.f, -size / 2.f, -size / 2.f,  size / 2.f };
        v[31] = { 0.251f, 0.333333f, 0.f, -1.f, 0.f, -size / 2.f, -size / 2.f, -size / 2.f };
        v[32] = { 0.499f, 0.333333f, 0.f, -1.f, 0.f,  size / 2.f, -size / 2.f, -size / 2.f };
        v[33] = { 0.251f, 0.f, 0.f, -1.f, 0.f, -size / 2.f, -size / 2.f,  size / 2.f };
        v[34] = { 0.499f, 0.333333f, 0.f, -1.f, 0.f,  size / 2.f, -size / 2.f, -size / 2.f };
        v[35] = { 0.499f, 0.f, 0.f, -1.f, 0.f,  size / 2.f, -size / 2.f,  size / 2.f };
    }


    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    MessageHandler::instance()->printDebug("Box: Generating VAO: %d\n", _vao);
    
    glGenBuffers(1, &_vbo);
    MessageHandler::instance()->printDebug("Box: Generating VBO: %d\n", _vbo);

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        v.size() * sizeof(helpers::VertexData),
        v.data(),
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
