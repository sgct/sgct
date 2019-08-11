/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/utils/SGCTSphere.h>

#include <sgct/ogl_headers.h>
#include <sgct/MessageHandler.h>
#include <sgct/Engine.h>
#include <glm/gtc/constants.hpp>
#include <algorithm>

namespace sgct_utils {

/*!
    This constructor requires a valid openGL contex
*/
SGCTSphere::SGCTSphere(float radius, unsigned int segments) {
    unsigned int vsegs = std::max<unsigned int>(segments, 2);
    unsigned int hsegs = vsegs*2;
    mNumberOfVertices = 1 + (vsegs - 1) * (hsegs + 1) + 1; // top + middle + bottom
    mNumberOfFaces = hsegs + (vsegs - 2) * hsegs * 2 + hsegs; // top + middle + bottom

    mVerts = new (std::nothrow) sgct_helpers::SGCTVertexData[mNumberOfVertices];
    memset(mVerts, 0, mNumberOfVertices * sizeof(sgct_helpers::SGCTVertexData));

    mIndices = new (std::nothrow) unsigned int[mNumberOfFaces * 3];
    if (mIndices == nullptr) {
        return;
    }

    memset(mIndices, 0, mNumberOfFaces * 3 * sizeof(unsigned int));

    // First vertex: top pole (+y is "up" in object local coords)
    addVertexData(0, 0.5f, 1.f, 0.f, 1.f, 0.f, 0.f, radius, 0.f);

    // Last vertex: bottom pole
    addVertexData(mNumberOfVertices - 1, 0.5f, 0.f, 0.f, -1.f, 0.f, 0.f, -radius, 0.f);

    // All other vertices:
    // vsegs-1 latitude rings of hsegs+1 vertices each (duplicates at texture seam s=0 / s=1)
    for (unsigned int j = 0; j < vsegs - 1; j++) {
        // vsegs-1 latitude rings of vertices
        double theta = (static_cast<double>(j + 1) / static_cast<double>(vsegs)) *
                        glm::pi<double>();
        float y = static_cast<float>(cos(theta));
        float R = static_cast<float>(sin(theta));

        for (unsigned int i = 0; i <= hsegs; i++) {
            // hsegs+1 vertices in each ring (duplicate for texcoords)
            double phi = (static_cast<double>(i) / static_cast<double>(hsegs)) *
                          glm::two_pi<double>();
            float x = R * static_cast<float>(cos(phi));
            float z = R * static_cast<float>(sin(phi));

            addVertexData(
                1 + j * (hsegs + 1) + i,
                static_cast<float>(i)/static_cast<float>(hsegs), //s
                1.f - static_cast<float>(j + 1) / static_cast<float>(vsegs), //t
                x, y, z, //normals
                radius * x,
                radius * y,
                radius * z
            );
        }
    }

    // The index array: triplets of integers, one for each triangle
    // Top cap
    for (unsigned int i = 0; i < hsegs; i++) {
        mIndices[3 * i] = 0;
        mIndices[3 * i + 2] = 1 + i;
        mIndices[3 * i + 1] = 2 + i;
    }
    // Middle part (possibly empty if vsegs=2)
    for (unsigned int j = 0; j < vsegs - 2; j++) {
        for (unsigned int i = 0; i < hsegs; i++) {
            unsigned int base = 3 * (hsegs + 2 * (j * hsegs + i));
            unsigned int i0 = 1 + j * (hsegs + 1) + i;
            mIndices[base] = i0;
            mIndices[base+1] = i0 + 1;
            mIndices[base+2] = i0 + hsegs + 1;
            mIndices[base+3] = i0 + hsegs + 1;
            mIndices[base+4] = i0 + 1;
            mIndices[base+5] = i0 + hsegs + 2;
        }
    }
    // Bottom cap
    for(unsigned int i = 0; i < hsegs; i++) {
        unsigned int base = 3 * (hsegs + 2 * (vsegs - 2) * hsegs);
        mIndices[base + 3 * i] = mNumberOfVertices - 1;
        mIndices[base + 3 * i + 2] = mNumberOfVertices - 2 - i;
        mIndices[base + 3 * i + 1] = mNumberOfVertices - 3 - i;
    }

    //create mesh
    createVBO();

    if (!sgct::Engine::checkForOGLErrors()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "SGCT Utils: Sphere creation error!\n"
        );
        void cleanup();
    }

    //free data
    if (mVerts != nullptr) {
        delete[] mVerts;
        mVerts = nullptr;
    }

    if (mIndices != nullptr) {
        delete[] mIndices;
        mIndices = nullptr;
    }
}

SGCTSphere::~SGCTSphere() {
    cleanUp();
}

void SGCTSphere::addVertexData(unsigned int pos, float s, float t, float nx, float ny,
                               float nz, float x, float y,  float z)
{
    mVerts[pos].set(s, t, nx, ny, nz, x, y, z);
}

void SGCTSphere::cleanUp() {
    //cleanup
    if (mVBO[0] != 0) {
        glDeleteBuffers(2, &mVBO[0]);
        mVBO[Vertex] = 0;
        mVBO[Index] = 0;
    }

    if (mVAO != 0) {
        glDeleteVertexArrays(1, &mVAO);
        mVAO = 0;
    }
}

/*!
    If openGL 3.3+ is used:
    layout 0 contains texture coordinates (vec2)
    layout 1 contains vertex normals (vec3)
    layout 2 contains vertex positions (vec3).
*/
void SGCTSphere::draw() {
    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        drawVBO();
    }
    else {
        drawVAO();
    }
}

void SGCTSphere::drawVBO() {
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);

    glBindBuffer(GL_ARRAY_BUFFER, mVBO[Vertex]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVBO[Index]);

    glInterleavedArrays(GL_T2F_N3F_V3F, 0, 0);
    glDrawElements(GL_TRIANGLES, mNumberOfFaces * 3, GL_UNSIGNED_INT, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glPopClientAttrib();
}

void SGCTSphere::drawVAO() {
    glBindVertexArray(mVAO);
    glDrawElements(GL_TRIANGLES, mNumberOfFaces * 3, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void SGCTSphere::createVBO() {
    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glGenVertexArrays(1, &mVAO);
        glBindVertexArray(mVAO);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "SGCTSphere: Generating VAO: %d\n", mVAO
        );
    }

    glGenBuffers(2, &mVBO[0]);
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "SGCTSphere: Generating VBOs: %d %d\n", mVBO[0], mVBO[1]
    );

    glBindBuffer(GL_ARRAY_BUFFER, mVBO[Vertex]);
    glBufferData(
        GL_ARRAY_BUFFER,
        mNumberOfVertices * sizeof(sgct_helpers::SGCTVertexData),
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

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVBO[Index]);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        mNumberOfFaces * 3 * sizeof(unsigned int),
        mIndices,
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
