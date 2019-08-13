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

/*!
    This constructor requires a valid openGL contex 
*/
sgct_utils::SGCTBox::SGCTBox(float size, TextureMappingMode tmm) {
    mVerts = new sgct_helpers::SGCTVertexData[36];
    memset(mVerts, 0, 36 * sizeof(sgct_helpers::SGCTVertexData));

    //populate the array
    if(tmm == Regular) {
        //A (front/+z)
        mVerts[0] = { 0.f, 1.f, 0.f, 0.f, 1.f, -size / 2.f,  size / 2.f, size / 2.f };
        mVerts[1] = { 0.f, 0.f, 0.f, 0.f, 1.f, -size / 2.f, -size / 2.f, size / 2.f };
        mVerts[2] = { 1.f, 0.f, 0.f, 0.f, 1.f,  size / 2.f, -size / 2.f, size / 2.f };
        mVerts[3] = { 0.f, 1.f, 0.f, 0.f, 1.f, -size / 2.f,  size / 2.f, size / 2.f };
        mVerts[4] = { 1.f, 0.f, 0.f, 0.f, 1.f,  size / 2.f, -size / 2.f, size / 2.f };
        mVerts[5] = { 1.f, 1.f, 0.f, 0.f, 1.f,  size / 2.f,  size / 2.f, size / 2.f };

        //B (right/+x)
        mVerts[ 6] = { 0.f, 1.f, 1.f, 0.f, 0.f, size  /2.f,  size / 2.f,  size / 2.f };
        mVerts[ 7] = { 0.f, 0.f, 1.f, 0.f, 0.f, size / 2.f, -size / 2.f,  size / 2.f };
        mVerts[ 8] = { 1.f, 0.f, 1.f, 0.f, 0.f, size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[ 9] = { 0.f, 1.f, 1.f, 0.f, 0.f, size / 2.f,  size / 2.f,  size / 2.f };
        mVerts[10] = { 1.f, 0.f, 1.f, 0.f, 0.f, size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[11] = { 1.f, 1.f, 1.f, 0.f, 0.f, size / 2.f,  size / 2.f, -size / 2.f };

        //C (Back/-z)
        mVerts[12] = { 0.f, 1.f, 0.f, 0.f, -1.f,  size / 2.f,  size / 2.f, -size / 2.f };
        mVerts[13] = { 0.f, 0.f, 0.f, 0.f, -1.f,  size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[14] = { 1.f, 0.f, 0.f, 0.f, -1.f, -size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[15] = { 0.f, 1.f, 0.f, 0.f, -1.f,  size / 2.f,  size / 2.f, -size / 2.f };
        mVerts[16] = { 1.f, 0.f, 0.f, 0.f, -1.f, -size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[17] = { 1.f, 1.f, 0.f, 0.f, -1.f, -size / 2.f,  size / 2.f, -size / 2.f };

        //D (Left/-x)
        mVerts[18] = { 0.f, 1.f, -1.f, 0.f, 0.f, -size / 2.f,  size / 2.f, -size / 2.f };
        mVerts[19] = { 0.f, 0.f, -1.f, 0.f, 0.f, -size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[20] = { 1.f, 0.f, -1.f, 0.f, 0.f, -size / 2.f, -size / 2.f,  size / 2.f };
        mVerts[21] = { 0.f, 1.f, -1.f, 0.f, 0.f, -size / 2.f,  size / 2.f, -size / 2.f };
        mVerts[22] = { 1.f, 0.f, -1.f, 0.f, 0.f, -size / 2.f, -size / 2.f,  size / 2.f };
        mVerts[23] = { 1.f, 1.f, -1.f, 0.f, 0.f, -size / 2.f,  size / 2.f,  size / 2.f };

        //E (Top/+y)
        mVerts[24] = { 0.f, 1.f, 0.f, 1.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        mVerts[25] = { 0.f, 0.f, 0.f, 1.f, 0.f, -size / 2.f, size / 2.f,  size / 2.f };
        mVerts[26] = { 1.f, 0.f, 0.f, 1.f, 0.f,  size / 2.f, size / 2.f,  size / 2.f };
        mVerts[27] = { 0.f, 1.f, 0.f, 1.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        mVerts[28] = { 1.f, 0.f, 0.f, 1.f, 0.f,  size / 2.f, size / 2.f,  size / 2.f };
        mVerts[29] = { 1.f, 1.f, 0.f, 1.f, 0.f,  size / 2.f, size / 2.f, -size / 2.f };

        //F (Bottom/-y)
        mVerts[30] = { 0.f, 1.f, 0.f, -1.f, 0.f, -size / 2.f, -size / 2.f,  size / 2.f };
        mVerts[31] = { 0.f, 0.f, 0.f, -1.f, 0.f, -size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[32] = { 1.f, 0.f, 0.f, -1.f, 0.f,  size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[33] = { 0.f, 1.f, 0.f, -1.f, 0.f, -size / 2.f, -size / 2.f,  size / 2.f };
        mVerts[34] = { 1.f, 0.f, 0.f, -1.f, 0.f,  size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[35] = { 1.f, 1.f, 0.f, -1.f, 0.f,  size / 2.f, -size / 2.f,  size / 2.f };
    }
    else if (tmm == CubeMap) {
        //A (front/+z)
        mVerts[0] = { 0.f, 1.f, 0.f, 0.f, 1.f, -size / 2.f, size / 2.f, size / 2.f };
        mVerts[1] = { 0.f, 0.5f, 0.f, 0.f, 1.f, -size / 2.f, -size / 2.f, size / 2.f };
        mVerts[2] = { 0.333333f, 0.5f, 0.f, 0.f, 1.f, size / 2.f, -size / 2.f, size / 2.f };
        mVerts[3] = { 0.f, 1.f, 0.f, 0.f, 1.f, -size / 2.f, size / 2.f, size / 2.f };
        mVerts[4] = { 0.333333f, 0.5f, 0.f, 0.f, 1.f, size / 2.f, -size / 2.f, size / 2.f };
        mVerts[5] = { 0.333333f, 1.f, 0.f, 0.f, 1.f, size / 2.f, size / 2.f, size / 2.f };

        //B (right/+x)
        mVerts[6] = { 0.333334f, 1.f, 1.f, 0.f, 0.f, size / 2.f, size / 2.f, size / 2.f };
        mVerts[7] = { 0.333334f, 0.5f, 1.f, 0.f, 0.f, size / 2.f, -size / 2.f, size / 2.f };
        mVerts[8] = { 0.666666f, 0.5f, 1.f, 0.f, 0.f, size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[9] = { 0.333334f, 1.f, 1.f, 0.f, 0.f, size / 2.f, size / 2.f, size / 2.f };
        mVerts[10] = { 0.666666f, 0.5f, 1.f, 0.f, 0.f, size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[11] = { 0.666666f, 1.f, 1.f, 0.f, 0.f, size / 2.f, size / 2.f, -size / 2.f };

        //C (Back/-z)
        mVerts[12] = { 0.666667f, 1.f, 0.f, 0.f, -1.f, size / 2.f, size / 2.f, -size / 2.f };
        mVerts[13] = { 0.666667f, 0.5f, 0.f, 0.f, -1.f, size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[14] = { 1.f, 0.5f, 0.f, 0.f, -1.f, -size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[15] = { 0.666667f, 1.f, 0.f, 0.f, -1.f, size / 2.f, size / 2.f, -size / 2.f };
        mVerts[16] = { 1.f, 0.5f, 0.f, 0.f, -1.f, -size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[17] = { 1.f, 1.f, 0.f, 0.f, -1.f, -size / 2.f, size / 2.f, -size / 2.f };

        //D (Left/-x)
        mVerts[18] = { 0.f, 0.5f, -1.f, 0.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        mVerts[19] = { 0.f, 0.f, -1.f, 0.f, 0.f, -size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[20] = { 0.333333f, 0.f, -1.f, 0.f, 0.f, -size / 2.f, -size / 2.f, size / 2.f };
        mVerts[21] = { 0.f, 0.5f, -1.f, 0.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        mVerts[22] = { 0.333333f, 0.f, -1.f, 0.f, 0.f, -size / 2.f, -size / 2.f, size / 2.f };
        mVerts[23] = { 0.333333f, 0.5f, -1.f, 0.f, 0.f, -size / 2.f, size / 2.f, size / 2.f };

        //E (Top/+y)
        mVerts[24] = { 0.333334f, 0.5f, 0.f, 1.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        mVerts[25] = { 0.333334f, 0.f, 0.f, 1.f, 0.f, -size / 2.f, size / 2.f, size / 2.f };
        mVerts[26] = { 0.666666f, 0.f, 0.f, 1.f, 0.f, size / 2.f, size / 2.f, size / 2.f };
        mVerts[27] = { 0.333334f, 0.5f, 0.f, 1.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        mVerts[28] = { 0.666666f, 0.f, 0.f, 1.f, 0.f, size / 2.f, size / 2.f, size / 2.f };
        mVerts[29] = { 0.666666f, 0.5f, 0.f, 1.f, 0.f, size / 2.f, size / 2.f, -size / 2.f };

        //F (Bottom/-y)
        mVerts[30] = { 0.666667f, 0.5f, 0.f, -1.f, 0.f, -size / 2.f, -size / 2.f, size / 2.f };
        mVerts[31] = { 0.666667f, 0.f, 0.f, -1.f, 0.f, -size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[32] = { 1.f, 0.f, 0.f, -1.f, 0.f, size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[33] = { 0.666667f, 0.5f, 0.f, -1.f, 0.f, -size / 2.f, -size / 2.f, size / 2.f };
        mVerts[34] = { 1.f, 0.f, 0.f, -1.f, 0.f, size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[35] = { 1.f, 0.5f, 0.f, -1.f, 0.f, size / 2.f, -size / 2.f, size / 2.f };
    }
    else  {
        //skybox
        //A (front/+z)
        mVerts[0] = { 1.f, 0.666666f, 0.f, 0.f, 1.f, -size / 2.f, size / 2.f, size / 2.f };
        mVerts[1] = { 1.f, 0.333334f, 0.f, 0.f, 1.f, -size / 2.f, -size / 2.f, size / 2.f };
        mVerts[2] = { 0.751f, 0.333334f, 0.f, 0.f, 1.f, size / 2.f, -size / 2.f, size / 2.f };
        mVerts[3] = { 1.f, 0.666666f, 0.f, 0.f, 1.f, -size / 2.f, size / 2.f, size / 2.f };
        mVerts[4] = { 0.751f, 0.333334f, 0.f, 0.f, 1.f, size / 2.f, -size / 2.f, size / 2.f };
        mVerts[5] = { 0.751f, 0.666666f, 0.f, 0.f, 1.f, size / 2.f, size / 2.f, size / 2.f };

        //B (right/+x)
        mVerts[6] = { 0.749f, 0.666666f, 1.f, 0.f, 0.f, size / 2.f, size / 2.f, size / 2.f };
        mVerts[7] = { 0.749f, 0.333334f, 1.f, 0.f, 0.f, size / 2.f, -size / 2.f, size / 2.f };
        mVerts[8] = { 0.501f, 0.333334f, 1.f, 0.f, 0.f, size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[9] = { 0.749f, 0.666666f, 1.f, 0.f, 0.f, size / 2.f, size / 2.f, size / 2.f };
        mVerts[10] = { 0.501f, 0.333334f, 1.f, 0.f, 0.f, size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[11] = { 0.501f, 0.666666f, 1.f, 0.f, 0.f, size / 2.f, size / 2.f, -size / 2.f };

        //C (Back/-z)
        mVerts[12] = { 0.499f, 0.666666f, 0.f, 0.f, -1.f,  size / 2.f,  size / 2.f, -size / 2.f };
        mVerts[13] = { 0.499f, 0.333334f, 0.f, 0.f, -1.f,  size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[14] = { 0.251f, 0.333334f, 0.f, 0.f, -1.f, -size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[15] = { 0.499f, 0.666666f, 0.f, 0.f, -1.f,  size / 2.f,  size / 2.f, -size / 2.f };
        mVerts[16] = { 0.251f, 0.333334f, 0.f, 0.f, -1.f, -size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[17] = { 0.251f, 0.666666f, 0.f, 0.f, -1.f, -size / 2.f,  size / 2.f, -size / 2.f };

        //D (Left/-x)
        mVerts[18] = { 0.249f, 0.666666f, -1.f, 0.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        mVerts[19] = { 0.249f, 0.333334f, -1.f, 0.f, 0.f, -size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[20] = { 0.000f, 0.333334f, -1.f, 0.f, 0.f, -size / 2.f, -size / 2.f, size / 2.f };
        mVerts[21] = { 0.249f, 0.666666f, -1.f, 0.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        mVerts[22] = { 0.000f, 0.333334f, -1.f, 0.f, 0.f, -size / 2.f, -size / 2.f, size / 2.f };
        mVerts[23] = { 0.000f, 0.666666f, -1.f, 0.f, 0.f, -size / 2.f, size / 2.f, size / 2.f };

        //E (Top/+y)
        mVerts[24] = { 0.251f, 0.666667f, 0.f, 1.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        mVerts[25] = { 0.251f, 1.f, 0.f, 1.f, 0.f, -size / 2.f, size / 2.f,  size / 2.f };
        mVerts[26] = { 0.499f, 1.f, 0.f, 1.f, 0.f,  size / 2.f, size / 2.f,  size / 2.f };
        mVerts[27] = { 0.251f, 0.666667f, 0.f, 1.f, 0.f, -size / 2.f, size / 2.f, -size / 2.f };
        mVerts[28] = { 0.499f, 1.f, 0.f, 1.f, 0.f,  size / 2.f, size / 2.f,  size / 2.f };
        mVerts[29] = { 0.499f, 0.666667f, 0.f, 1.f, 0.f,  size / 2.f, size / 2.f, -size / 2.f };

        //F (Bottom/-y)
        mVerts[30] = { 0.251f, 0.f, 0.f, -1.f, 0.f, -size / 2.f, -size / 2.f,  size / 2.f };
        mVerts[31] = { 0.251f, 0.333333f, 0.f, -1.f, 0.f, -size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[32] = { 0.499f, 0.333333f, 0.f, -1.f, 0.f,  size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[33] = { 0.251f, 0.f, 0.f, -1.f, 0.f, -size / 2.f, -size / 2.f,  size / 2.f };
        mVerts[34] = { 0.499f, 0.333333f, 0.f, -1.f, 0.f,  size / 2.f, -size / 2.f, -size / 2.f };
        mVerts[35] = { 0.499f, 0.f, 0.f, -1.f, 0.f,  size / 2.f, -size / 2.f,  size / 2.f };
    }

    createVBO();

    if (!sgct::Engine::checkForOGLErrors()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,\
            "SGCT Utils: Box creation error!\n"
        );
        void cleanup();
    }

    //free data
    if (mVerts != nullptr) {
        delete [] mVerts;
        mVerts = nullptr;
    }
}

SGCTBox::~SGCTBox() {
    cleanUp();
}

/*!
    If openGL 3.3+ is used:
    layout 0 contains texture coordinates (vec2)
    layout 1 contains vertex normals (vec3)
    layout 2 contains vertex positions (vec3).
*/
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

void SGCTBox::cleanUp() {
    //cleanup
    if (mVBO != 0) {
        glDeleteBuffers(1, &mVBO);
        mVBO = 0;
    }

    if (mVAO != 0) {
        glDeleteVertexArrays(1, &mVAO);
        mVAO = 0;
    }
}

void SGCTBox::createVBO() {
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
        36 * sizeof(sgct_helpers::SGCTVertexData),
        mVerts,
        GL_STATIC_DRAW
    );

    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        //texcoords
        glVertexAttribPointer(
            0,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(sgct_helpers::SGCTVertexData),
            reinterpret_cast<void*>(0)
        );
        //normals
        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(sgct_helpers::SGCTVertexData),
            reinterpret_cast<void*>(8)
        );
        //vert positions
        glVertexAttribPointer(
            2,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(sgct_helpers::SGCTVertexData),
            reinterpret_cast<void*>(20)
        );
    }

    //unbind
    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glBindVertexArray(0);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace sgct_utils
