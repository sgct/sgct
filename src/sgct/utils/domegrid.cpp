/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/utils/domegrid.h>

#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/ogl_headers.h>
#include <glm/gtc/constants.hpp>

namespace sgct_utils {

DomeGrid::DomeGrid(float radius, float FOV, unsigned int segments,
                           unsigned int rings, unsigned int resolution)
    : mResolution(resolution)
    , mRings(rings)
    , mSegments(segments)
{
    // must be four or higher
    if (mResolution < 4) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Warning,
            "Warning: Dome geometry resolution must be higher than 4\n"
        );
        mResolution = 4;
    }

    createVBO(radius, FOV);

    // if error occured
    if (!sgct::Engine::checkForOGLErrors()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "SGCT Utils: Dome creation error\n"
        );
    }
}

DomeGrid::~DomeGrid() {
    glDeleteBuffers(1, &mVBO);
    mVBO = 0;

    glDeleteVertexArrays(1, &mVAO);
    mVAO = 0;
}

void DomeGrid::draw() {
    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        drawVBO();
    }
    else {
        drawVAO();
    }
}

void DomeGrid::drawVBO() {
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_VERTEX_ARRAY);

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);

    glVertexPointer(3, GL_FLOAT, 0, nullptr);

    for (unsigned int r = 0; r < mRings; r++) {
        glDrawArrays(GL_LINE_LOOP, r * mResolution, mResolution);
    }
    for (unsigned int s = 0; s < mSegments; s++) {
        glDrawArrays(
            GL_LINE_STRIP,
            mRings * mResolution + s * ((mResolution / 4) + 1),
            (mResolution / 4) + 1
        );
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPopClientAttrib();
}

void DomeGrid::drawVAO() {
    glBindVertexArray(mVAO);

    for (unsigned int r = 0; r < mRings; r++) {
        glDrawArrays(GL_LINE_LOOP, r * mResolution, mResolution);
    }
    for (unsigned int s = 0; s < mSegments; s++) {
        glDrawArrays(
            GL_LINE_STRIP,
            mRings * mResolution + s * ((mResolution / 4) + 1),
            (mResolution / 4) + 1
        );
    }

    glBindVertexArray(0);
}

void DomeGrid::createVBO(float radius, float FOV) {
    const unsigned int numberOfVertices = (mSegments * ((mResolution / 4) + 1) +
                                           mRings * mResolution) * 6;
    std::vector<float> verts(numberOfVertices, 0.f);

    unsigned int pos = 0;

    // create rings
    for (unsigned int r = 1; r <= mRings; r++) {
        const float elevationAngle = glm::radians<float>(
            (FOV / 2.f) * (static_cast<float>(r) / static_cast<float>(mRings))
        );
        for (unsigned int i = 0; i < mResolution; i++) {
            const float theta = glm::two_pi<float>() *
                (static_cast<float>(i) / static_cast<float>(mResolution));

            verts[pos] = radius * sinf(elevationAngle) * cosf(theta);
            verts[pos + 1] = radius * cosf(elevationAngle);
            verts[pos + 2] = radius * sinf(elevationAngle) * sinf(theta);
            pos += 3;
        }
    }

    // create segments
    for (unsigned int s = 0; s < mSegments; s++) {
        const float theta = glm::two_pi<float>() *
            (static_cast<float>(s) / static_cast<float>(mSegments));

        for (unsigned int i = 0; i < (mResolution / 4) + 1; i++) {
            const float elevationAngle = glm::radians<float>(FOV / 2.f) *
                (static_cast<float>(i) / static_cast<float>(mResolution / 4));

            verts[pos] = radius * sinf(elevationAngle) * cosf(theta);
            verts[pos + 1] = radius * cosf(elevationAngle);
            verts[pos + 2] = radius * sinf(elevationAngle) * sinf(theta);
            pos += 3;
        }
    }



    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glGenVertexArrays(1, &mVAO);
        glBindVertexArray(mVAO);
        glEnableVertexAttribArray(0);
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "DomeGrid: Generating VAO: %d\n", mVAO
        );
    }

    glGenBuffers(1, &mVBO);
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "DomeGrid: Generating VBO: %d\n", mVBO
    );

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        verts.size() * sizeof(float),
        verts.data(),
        GL_STATIC_DRAW
    );

    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            0,
            nullptr
        );
    }

    // unbind
    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glBindVertexArray(0);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace sgct_utils
