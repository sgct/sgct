/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/utils/SGCTPlane.h>
#include <sgct/ogl_headers.h>
#include <sgct/MessageHandler.h>
#include <sgct/Engine.h>

namespace sgct_utils {

/*!
    This constructor requires a valid openGL contex 
*/
SGCTPlane::SGCTPlane(float width, float height) {
    mVerts = new sgct_helpers::SGCTVertexData[4];
    memset(mVerts, 0, 4 * sizeof(sgct_helpers::SGCTVertexData));

    //populate the array
    mVerts[0].set(0.f, 0.f, 0.f, 0.f, 1.f, -width / 2.f, -height / 2.f, 0.f);
    mVerts[1].set(1.f, 0.f, 0.f, 0.f, 1.f,  width / 2.f, -height / 2.f, 0.f);
    mVerts[2].set(0.f, 1.f, 0.f, 0.f, 1.f, -width / 2.f,  height / 2.f, 0.f);
    mVerts[3].set(1.f, 1.f, 0.f, 0.f, 1.f,  width / 2.f,  height / 2.f, 0.f);
        
    createVBO();

    if (!sgct::Engine::checkForOGLErrors()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::NOTIFY_ERROR,
            "SGCT Utils: Plane creation error!\n"
        );
        void cleanup();
    }

    //free data
    if (mVerts != nullptr) {
        delete [] mVerts;
        mVerts = nullptr;
    }
}

SGCTPlane::~SGCTPlane() {
    cleanUp();
}

/*!
    If openGL 3.3+ is used:
    layout 0 contains texture coordinates (vec2)
    layout 1 contains vertex normals (vec3)
    layout 2 contains vertex positions (vec3).
*/
void SGCTPlane::draw() {
    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        drawVBO();
    }
    else {
        drawVAO();
    }
}

void SGCTPlane::drawVBO() {
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

void SGCTPlane::drawVAO() {
    glBindVertexArray(mVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    //unbind
    glBindVertexArray(0);
}

void SGCTPlane::cleanUp() {
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

void SGCTPlane::createVBO() {
    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glGenVertexArrays(1, &mVAO);
        glBindVertexArray(mVAO);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::NOTIFY_DEBUG,
            "SGCTPlane: Generating VAO: %d\n",
            mVAO
        );
    }
    
    glGenBuffers(1, &mVBO);
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::NOTIFY_DEBUG,
        "SGCTPlane: Generating VBO: %d\n",
        mVBO
    );

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        4 * sizeof(sgct_helpers::SGCTVertexData),
        mVerts,
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
