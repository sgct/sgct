/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/utils/SGCTBox.h>

#include <sgct/Engine.h>
#include <sgct/MessageHandler.h>
#include <sgct/ogl_headers.h>
#include <sgct/helpers/SGCTVertexData.h>

namespace sgct_utils {

SGCTBox::SGCTBox(float size, TextureMappingMode mode) {
    createVBO(size, mode);

    if (!sgct::Engine::checkForOGLErrors()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,\
            "SGCT Utils: Box creation error\n"
        );
    }
}

SGCTBox::~SGCTBox() {
    glDeleteBuffers(1, &mVBO);
    mVBO = 0;

    glDeleteVertexArrays(1, &mVAO);
    mVAO = 0;
}

void SGCTBox::draw() {
    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        drawVBO();
    }
    else {
        drawVAO();
    }
}

void SGCTBox::drawVBO() {
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    
    glInterleavedArrays(GL_T2F_N3F_V3F, 0, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glPopClientAttrib();
}

void SGCTBox::drawVAO() {
    glBindVertexArray( mVAO );
    glDrawArrays(GL_TRIANGLES, 0, 36);

    //unbind
    glBindVertexArray(0);
}

void SGCTBox::createVBO(float size, TextureMappingMode tmm) {
    std::vector<sgct_helpers::SGCTVertexData> v(36);

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


    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glGenVertexArrays(1, &mVAO);
        glBindVertexArray( mVAO );
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "SGCTBox: Generating VAO: %d\n", mVAO
        );
    }
    
    glGenBuffers(1, &mVBO);
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "SGCTBox: Generating VBO: %d\n", mVBO
    );

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        v.size() * sizeof(sgct_helpers::SGCTVertexData),
        v.data(),
        GL_STATIC_DRAW
    );

    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        // texcoords
        glVertexAttribPointer(
            0,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(sgct_helpers::SGCTVertexData),
            reinterpret_cast<void*>(0)
        );

        // normals
        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(sgct_helpers::SGCTVertexData),
            reinterpret_cast<void*>(8)
        );

        // vert positions
        glVertexAttribPointer(
            2,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(sgct_helpers::SGCTVertexData),
            reinterpret_cast<void*>(20)
        );
    }

    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glBindVertexArray(0);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace sgct_utils
