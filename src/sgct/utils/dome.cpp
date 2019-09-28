/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/utils/dome.h>

#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/ogl_headers.h>
#include <glm/gtc/constants.hpp>

namespace sgct::utils {

Dome::Dome(float radius, float FOV, unsigned int azimuthSteps,
           unsigned int elevationSteps)
    : mElevationSteps(elevationSteps)
    , mAzimuthSteps(azimuthSteps)
{
    std::vector<helpers::VertexData> vertices;

    // must be four or higher
    if (mAzimuthSteps < 4) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Warning: Dome geometry azimuth steps must be exceed 4\n"
        );
        mAzimuthSteps = 4;
    }

    // must be four or higher
    if (mElevationSteps < 4)  {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Warning: Dome geometry elevation steps must be exceed 4\n"
        );
        mElevationSteps = 4;
    }


    createVBO(radius, FOV);

    if (!Engine::checkForOGLErrors()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "SGCT Utils: Dome creation error\n"
        );
    }
}

Dome::~Dome() {
    glDeleteBuffers(1, &mVBO);
    glDeleteBuffers(1, &mIBO);
    glDeleteVertexArrays(1, &mVAO);
}

void Dome::draw() {
    drawVAO();
}

void Dome::drawVBO() {
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

void Dome::drawVAO() {
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
    const unsigned int size = mAzimuthSteps + 2; 
    const unsigned int offset = (2 * mAzimuthSteps + 2) * (mElevationSteps - 1);
    glDrawElements(
        GL_TRIANGLE_FAN,
        size,
        GL_UNSIGNED_INT,
        reinterpret_cast<void*>(offset * sizeof(unsigned int))
    );
    glBindVertexArray(0);
}

void Dome::createVBO(float radius, float FOV) {
    const float lift = (180.f - FOV) / 2.f;

    std::vector<helpers::VertexData> verts;
    std::vector<unsigned int> indices;

    for (int a = 0; a < mAzimuthSteps; a++) {
        const float azimuth = glm::radians(
            static_cast<float>(a * 360.f) / static_cast<float>(mAzimuthSteps)
        );

        const float elevation = glm::radians(lift);
        const float x = cos(elevation) * sin(azimuth);
        const float y = sin(elevation);
        const float z = -cos(elevation) * cos(azimuth);
        const float s =  sin(azimuth) * 0.5f + 0.5f;
        const float t = -cos(azimuth) * 0.5f + 0.5f;

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

            const float x = cos(elevation) * sin(azimuth);
            const float z = -cos(elevation) * cos(azimuth);

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
            });

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
    const float y = sin(elevation);
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


    glGenVertexArrays(1, &mVAO);
    glBindVertexArray(mVAO);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "Dome: Generating VAO: %d\n", mVAO
    );

    glGenBuffers(2, &mVBO);
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "Dome: Generating VBOs: %d %d\n", mVBO, mIBO
    );

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<int>(verts.size()) * sizeof(helpers::VertexData),
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

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<int>(indices.size()) * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW
    );

    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace sgct::utils
