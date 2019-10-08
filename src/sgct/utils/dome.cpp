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
    : _elevationSteps(elevationSteps)
    , _azimuthSteps(azimuthSteps)
{
    std::vector<helpers::VertexData> vertices;

    // must be four or higher
    if (_azimuthSteps < 4) {
        MessageHandler::instance()->printWarning(
            "Warning: Dome geometry azimuth steps must be exceed 4"
        );
        _azimuthSteps = 4;
    }

    // must be four or higher
    if (_elevationSteps < 4)  {
        MessageHandler::instance()->printWarning(
            "Warning: Dome geometry elevation steps must be exceed 4"
        );
        _elevationSteps = 4;
    }


    createVBO(radius, FOV);

    if (!Engine::checkForOGLErrors()) {
        MessageHandler::instance()->printError("SGCT Utils: Dome creation error");
    }
}

Dome::~Dome() {
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ibo);
    glDeleteVertexArrays(1, &_vao);
}

void Dome::draw() {
    drawVAO();
}

void Dome::drawVBO() {
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);

    glInterleavedArrays(GL_T2F_N3F_V3F, 0, 0);
    
    for (int i = 0; i < _elevationSteps - 1; i++) {
        const unsigned int size = (2 * _azimuthSteps + 2);
        const unsigned int offset = i * size;
        glDrawElements(
            GL_TRIANGLE_STRIP,
            size,
            GL_UNSIGNED_INT,
            reinterpret_cast<void*>(offset * sizeof(unsigned int))
        );
    }

    // one extra for the cap vertex and one extra for duplication of last index
    const unsigned int size = _azimuthSteps + 2;
    const unsigned int offset = (2 * _azimuthSteps + 2) * (_elevationSteps - 1);
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
    glBindVertexArray(_vao);

    for (int i = 0; i < _elevationSteps - 1; i++) {
        const unsigned int size = (2 * _azimuthSteps + 2);
        const unsigned int offset = i * size;
        glDrawElements(
            GL_TRIANGLE_STRIP,
            size,
            GL_UNSIGNED_INT,
            reinterpret_cast<void*>(offset * sizeof(unsigned int))
        );
    }

    // one extra for the cap vertex and one extra for duplication of last index
    const unsigned int size = _azimuthSteps + 2; 
    const unsigned int offset = (2 * _azimuthSteps + 2) * (_elevationSteps - 1);
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

    for (int a = 0; a < _azimuthSteps; a++) {
        const float azimuth = glm::radians(
            static_cast<float>(a * 360.f) / static_cast<float>(_azimuthSteps)
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
    for (int e = 1; e <= _elevationSteps - 1; e++) {
        const float de = static_cast<float>(e) / static_cast<float>(_elevationSteps);
        const float elevation = glm::radians(lift + de * (90.0f - lift));

        const float y = sin(elevation);

        for (int a = 0; a < _azimuthSteps; a++) {
            const float azimuth = glm::radians(
                static_cast<float>(a * 360.f) / static_cast<float>(_azimuthSteps)
            );

            const float x = cos(elevation) * sin(azimuth);
            const float z = -cos(elevation) * cos(azimuth);

            float s = (static_cast<float>(_elevationSteps - e) /
                       static_cast<float>(_elevationSteps)) * sin(azimuth);
            float t = (static_cast<float>(_elevationSteps - e) /
                       static_cast<float>(_elevationSteps)) * -cos(azimuth);
            s = s * 0.5f + 0.5f;
            t = t * 0.5f + 0.5f;

            verts.push_back({
                s, t,
                x, y, z,
                x * radius, y * radius, z * radius
            });

            indices.push_back(numVerts);
            indices.push_back(_azimuthSteps + numVerts);
            ++numVerts;
        }

        indices.push_back(numVerts - _azimuthSteps);
        indices.push_back(numVerts);
    }

    const int e = _elevationSteps;
    const float de = static_cast<float>(e) / static_cast<float>(_elevationSteps);
    const float elevation = glm::radians(lift + de * (90.f - lift));
    const float y = sin(elevation);
    verts.push_back({
        0.5f, 0.5f,
        0.f, 1.f, 0.f,
        0.f, y * radius, 0.f
    });


    indices.push_back(numVerts + _azimuthSteps);
    for (int a = 1; a <= _azimuthSteps; a++) {
        indices.push_back(numVerts + _azimuthSteps - a);
    }
    indices.push_back(numVerts + _azimuthSteps - 1);


    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    MessageHandler::instance()->printDebug("Dome: Generating VAO: %d", _vao);

    glGenBuffers(2, &_vbo);
    MessageHandler::instance()->printDebug("Dome: Generating VBOs: %d %d", _vbo, _ibo);

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
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

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
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
