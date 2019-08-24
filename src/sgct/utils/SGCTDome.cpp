/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/utils/SGCTDome.h>

#include <sgct/Engine.h>
#include <sgct/MessageHandler.h>
#include <sgct/ogl_headers.h>
#include <glm/gtc/constants.hpp>

namespace sgct_utils {

SGCTDome::SGCTDome(float radius, float FOV, unsigned int azimuthSteps,
                   unsigned int elevationSteps)
    : mElevationSteps(elevationSteps)
    , mAzimuthSteps(azimuthSteps)
{
    std::vector<sgct_helpers::SGCTVertexData> vertices;

    // must be four or higher
    if (mAzimuthSteps < 4) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Warning,
            "Warning: Dome geometry azimuth steps must be exceed 4\n"
        );
        mAzimuthSteps = 4;
    }

    // must be four or higher
    if (mElevationSteps < 4)  {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Warning,
            "Warning: Dome geometry elevation steps must be exceed 4\n"
        );
        mElevationSteps = 4;
    }


    createVBO(radius, FOV);

    if (!sgct::Engine::checkForOGLErrors()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "SGCT Utils: Dome creation error\n"
        );
    }
}

SGCTDome::~SGCTDome() {
    glDeleteBuffers(1, &mVBO);
    mVBO = 0;
    glDeleteBuffers(1, &mIBO);
    mIBO = 0;
    glDeleteVertexArrays(1, &mVAO);
    mVAO = 0;
}

void SGCTDome::draw() {
    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        drawVBO();
    }
    else {
        drawVAO();
    }
}

void SGCTDome::drawVBO() {
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);

    glInterleavedArrays(GL_T2F_N3F_V3F, 0, 0);
    
    for (int i = 0; i < mElevationSteps - 1; i++) {
        const unsigned int size = (2 * mAzimuthSteps + 2);
        const unsigned int offset = i * size;
        glDrawElements(
            GL_TRIANGLE_STRIP,
            size,
            GL_UNSIGNED_INT,
            reinterpret_cast<void*>(offset * sizeof(unsigned int))
        );
    }

    // one extra for the cap vertex and one extra for duplication of last index
    const unsigned int size = mAzimuthSteps + 2;
    const unsigned int offset = (2 * mAzimuthSteps + 2) * (mElevationSteps - 1);
    glDrawElements(
        GL_TRIANGLE_FAN,
        size,
        GL_UNSIGNED_INT,
        reinterpret_cast<void*>(offset * sizeof(unsigned int))
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glPopClientAttrib();
}

void SGCTDome::drawVAO() {
    glBindVertexArray(mVAO);

    for (int i = 0; i < mElevationSteps - 1; i++) {
        const unsigned int size = (2 * mAzimuthSteps + 2);
        const unsigned int offset = i * size;
        glDrawElements(
            GL_TRIANGLE_STRIP,
            size,
            GL_UNSIGNED_INT,
            reinterpret_cast<void*>(offset * sizeof(unsigned int))
        );
    }

    // one extra for the cap vertex and one extra for duplication of last index
    unsigned int size = mAzimuthSteps + 2; 
    unsigned int offset = (2 * mAzimuthSteps + 2) * (mElevationSteps - 1);
    glDrawElements(
        GL_TRIANGLE_FAN,
        size,
        GL_UNSIGNED_INT,
        reinterpret_cast<void*>(offset*sizeof(unsigned int))
    );
    glBindVertexArray(0);
}

void SGCTDome::createVBO(float radius, float FOV) {
    const float lift = (180.f - FOV) / 2.f;

    std::vector<sgct_helpers::SGCTVertexData> verts;
    std::vector<unsigned int> indices;

    for (int a = 0; a < mAzimuthSteps; a++) {
        const float azimuth = glm::radians(
            static_cast<float>(a * 360.f) / static_cast<float>(mAzimuthSteps)
        );

        const float elevation = glm::radians(lift);
        const float x = cosf(elevation) * sinf(azimuth);
        const float y = sin(elevation);
        const float z = -cosf(elevation) * cosf(azimuth);
        const float s =  sinf(azimuth) * 0.5f + 0.5f;
        const float t = -cosf(azimuth) * 0.5f + 0.5f;

        verts.push_back({
            s, t,
            x, y, z,
            x * radius, y * radius, z * radius
        });
    }

    int numVerts = 0;
    for (int e = 1; e <= mElevationSteps - 1; e++) {
        const float de = static_cast<float>(e) / static_cast<float>(mElevationSteps);
        const float elevation = glm::radians(lift + de * (90.0f - lift));

        const float y = sin(elevation);

        for (int a = 0; a < mAzimuthSteps; a++) {
            const float azimuth = glm::radians(
                static_cast<float>(a * 360.f) / static_cast<float>(mAzimuthSteps)
            );

            const float x = cosf(elevation) * sinf(azimuth);
            const float z = -cosf(elevation) * cosf(azimuth);

            float s = (static_cast<float>(mElevationSteps - e) /
                       static_cast<float>(mElevationSteps)) * sin(azimuth);
            float t = (static_cast<float>(mElevationSteps - e) /
                       static_cast<float>(mElevationSteps)) * -cos(azimuth);
            s = s * 0.5f + 0.5f;
            t = t * 0.5f + 0.5f;

            verts.push_back({
                s, t,
                x, y, z,
                x * radius, y * radius, z * radius
                }
            );

            indices.push_back(numVerts);
            indices.push_back(mAzimuthSteps + numVerts);
            ++numVerts;
        }

        indices.push_back(numVerts - mAzimuthSteps);
        indices.push_back(numVerts);
    }

    const int e = mElevationSteps;
    const float de = static_cast<float>(e) / static_cast<float>(mElevationSteps);
    const float elevation = glm::radians(lift + de * (90.f - lift));
    const float y = sinf(elevation);
    verts.push_back({
        0.5f, 0.5f,
        0.f, 1.f, 0.f,
        0.f, y * radius, 0.f
    });


    indices.push_back(numVerts + mAzimuthSteps);
    for (int a = 1; a <= mAzimuthSteps; a++) {
        indices.push_back(numVerts + mAzimuthSteps - a);
    }
    indices.push_back(numVerts + mAzimuthSteps - 1);


    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glGenVertexArrays(1, &mVAO);
        glBindVertexArray(mVAO);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "SGCTDome: Generating VAO: %d\n", mVAO
        );
    }

    glGenBuffers(2, &mVBO);
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "SGCTDome: Generating VBOs: %d %d\n", mVBO, mIBO
    );

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<int>(verts.size()) * sizeof(sgct_helpers::SGCTVertexData),
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

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<int>(indices.size()) * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW
    );

    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glBindVertexArray(0);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace sgct_utils
