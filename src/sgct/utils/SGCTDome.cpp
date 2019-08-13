/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/utils/SGCTDome.h>

#include <sgct/Engine.h>
#include <sgct/ogl_headers.h>
#include <sgct/MessageHandler.h>
#include <glm/gtc/constants.hpp>

namespace sgct_utils {

/*!
    This constructor requires a valid openGL contex
*/
SGCTDome::SGCTDome(float radius, float FOV, unsigned int azimuthSteps,
                   unsigned int elevationSteps)
    : mElevationSteps(elevationSteps)
    , mAzimuthSteps(azimuthSteps)
{
    float lift = (180.0f - FOV) / 2.0f;
    std::vector<sgct_helpers::SGCTVertexData> vertices;

    // must be four or higher
    if (mAzimuthSteps < 4) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Warning,
            "Warning: Dome geometry azimuth steps must be exceed 4.\n"
        );
        mAzimuthSteps = 4;
    }

    // must be four or higher
    if (mElevationSteps < 4)  {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Warning,
            "Warning: Dome geometry elevation steps must be exceed 4.\n"
        );
        mElevationSteps = 4;
    }

    int e, numVerts = 0;
    float x, z;
    float s, t;
    float azimuth;
    e = 0;

    // delta elevation
    float de = static_cast<float>(e) / static_cast<float>(mElevationSteps);
    float elevation = glm::radians(lift + de * (90.f - lift));
    float y = sinf(elevation);

    for (int a = 0; a < mAzimuthSteps; a++) {
        azimuth = glm::radians(
            static_cast<float>(a * 360) / static_cast<float>(mAzimuthSteps)
        );

        x = cosf(elevation) * sinf(azimuth);
        z = -cosf(elevation) * cosf(azimuth);

        s = (static_cast<float>(mElevationSteps - e) / static_cast<float>(mElevationSteps)) * sinf(azimuth);
        t = (static_cast<float>(mElevationSteps - e) / static_cast<float>(mElevationSteps)) * -cosf(azimuth);
        s = s * 0.5f + 0.5f;
        t = t * 0.5f + 0.5f;

        sgct_helpers::SGCTVertexData vertex;
        vertex.x = x * radius;
        vertex.y = y * radius;
        vertex.z = z * radius;
        vertex.nx = x;
        vertex.ny = y;
        vertex.nz = z;
        vertex.s = s;
        vertex.t = t;
        mVerts.push_back(std::move(vertex));
    }

    for (e = 1; e <= mElevationSteps - 1; e++) {
        de = static_cast<float>(e) / static_cast<float>(mElevationSteps);
        elevation = glm::radians(lift + de * (90.0f - lift));

        y = sinf(elevation);

        for (int a = 0; a < mAzimuthSteps; a++) {
            azimuth = glm::radians(static_cast<float>(a * 360) / static_cast<float>(mAzimuthSteps));

            x = cosf(elevation) * sinf(azimuth);
            z = -cosf(elevation) * cosf(azimuth);

            s = (static_cast<float>(mElevationSteps - e) / static_cast<float>(mElevationSteps)) * sinf(azimuth);
            t = (static_cast<float>(mElevationSteps - e) / static_cast<float>(mElevationSteps)) * -cosf(azimuth);
            s = s * 0.5f + 0.5f;
            t = t * 0.5f + 0.5f;

            sgct_helpers::SGCTVertexData vertex;
            vertex.x = x * radius;
            vertex.y = y * radius;
            vertex.z = z * radius;
            vertex.nx = x;
            vertex.ny = y;
            vertex.nz = z;
            vertex.s = s;
            vertex.t = t;
            mVerts.push_back(std::move(vertex));

            mIndices.push_back(numVerts);
            mIndices.push_back(mAzimuthSteps + numVerts++);
        }

        mIndices.push_back(numVerts - mAzimuthSteps);
        mIndices.push_back(numVerts);
    }

    e = mElevationSteps;
    de = static_cast<float>(e) / static_cast<float>(mElevationSteps);
    elevation = glm::radians(lift + de * (90.f - lift));

    y = sinf(elevation);

    sgct_helpers::SGCTVertexData vertex;
    vertex.x = 0.f;
    vertex.y = y * radius;
    vertex.z = 0.f;
    vertex.nx = 0.f;
    vertex.ny = 1.f;
    vertex.nz = 0.f;
    vertex.s = 0.5f;
    vertex.t = 0.5f;
    mVerts.push_back(std::move(vertex));

    mIndices.push_back(numVerts + mAzimuthSteps);

    for (int a = 1; a <= mAzimuthSteps; a++) {
        mIndices.push_back(numVerts + mAzimuthSteps - a);
    }
    mIndices.push_back(numVerts + mAzimuthSteps - 1);

    createVBO();

    if (!sgct::Engine::checkForOGLErrors()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "SGCT Utils: Dome creation error!\n"
        );
        cleanup();
    }

    // free data
    mVerts.clear();
    mIndices.clear();
}

SGCTDome::~SGCTDome() {
    cleanup();
}

void SGCTDome::cleanup() {
    //cleanup
    glDeleteBuffers(2, &mVBO[0]);
    mVBO[Vertex] = 0;
    mVBO[Index] = 0;

    glDeleteVertexArrays(1, &mVAO);
    mVAO = 0;
}

/*!
If openGL 3.3+ is used:
layout 0 contains texture coordinates (vec2)
layout 1 contains vertex normals (vec3)
layout 2 contains vertex positions (vec3).
*/
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

    glBindBuffer(GL_ARRAY_BUFFER, mVBO[Vertex]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVBO[Index]);

    glInterleavedArrays(GL_T2F_N3F_V3F, 0, 0);
    
    for (int n = 0; n < mElevationSteps - 1; n++) {
        unsigned int size = (2 * mAzimuthSteps + 2);
        unsigned int offset = n * size;
        glDrawElements(
            GL_TRIANGLE_STRIP,
            size,
            GL_UNSIGNED_INT,
            reinterpret_cast<void*>(offset * sizeof(unsigned int))
        );
    }

    //one extra for the cap vertex and one extra for duplication of last index
    unsigned int size = mAzimuthSteps + 2;
    unsigned int offset = (2 * mAzimuthSteps + 2) * (mElevationSteps - 1);
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

    for (int n = 0; n < mElevationSteps - 1; n++) {
        unsigned int size = (2 * mAzimuthSteps + 2);
        unsigned int offset = n * size;
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

void SGCTDome::createVBO() {
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

    glGenBuffers(2, &mVBO[0]);
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "SGCTDome: Generating VBOs: %d %d\n", mVBO[0], mVBO[1]
    );

    glBindBuffer(GL_ARRAY_BUFFER, mVBO[Vertex]);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<int>(mVerts.size()) * sizeof(sgct_helpers::SGCTVertexData),
        mVerts.data(),
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

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVBO[Index]);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<int>(mIndices.size()) * sizeof(unsigned int),
        mIndices.data(),
        GL_STATIC_DRAW
    );

    //unbind
    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glBindVertexArray(0);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace sgct_utils
