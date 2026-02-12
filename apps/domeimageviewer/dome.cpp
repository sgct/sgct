/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include "dome.h"

#include <sgct/log.h>
#include <sgct/opengl.h>
#include <glm/glm.hpp>

Dome::Dome(float r, float FOV, unsigned int azimuthSteps, unsigned int elevationSteps)
    : _elevationSteps(elevationSteps)
    , _azimuthSteps(azimuthSteps)
{
    struct VertexData {
        float s = 0.f;
        float t = 0.f;
        float nx = 0.f;
        float ny = 0.f;
        float nz = 0.f;
        float x = 0.f;
        float y = 0.f;
        float z = 0.f;
    };

    glCreateBuffers(1, &_vbo);
    glCreateBuffers(1, &_ibo);
    glCreateVertexArrays(1, &_vao);
    glVertexArrayVertexBuffer(_vao, 0, _vbo, 0, sizeof(VertexData));
    glVertexArrayElementBuffer(_vao, _ibo);

    glEnableVertexArrayAttrib(_vao, 0);
    glVertexArrayAttribFormat(_vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(_vao, 0, 0);

    glEnableVertexArrayAttrib(_vao, 1);
    glVertexArrayAttribFormat(_vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(VertexData, nx));
    glVertexArrayAttribBinding(_vao, 1, 0);

    glEnableVertexArrayAttrib(_vao, 2);
    glVertexArrayAttribFormat(_vao, 2, 3, GL_FLOAT, GL_FALSE, offsetof(VertexData, x));
    glVertexArrayAttribBinding(_vao, 2, 0);

    if (_azimuthSteps < 4) {
        sgct::Log::Warning("Azimuth steps must be higher than 4");
    }
    if (_elevationSteps < 4)  {
        sgct::Log::Warning("Elevation steps must be higher than 4");
    }

    // Create VAO
    const float lift = (180.f - FOV) / 2.f;

    std::vector<VertexData> verts;
    std::vector<unsigned int> indices;

    for (int a = 0; a < _azimuthSteps; a++) {
        const float azimuth = glm::radians((a * 360.f) / _azimuthSteps);

        const float elevation = glm::radians(lift);
        const float x = std::cos(elevation) * std::sin(azimuth);
        const float y = std::sin(elevation);
        const float z = -std::cos(elevation) * std::cos(azimuth);
        const float s = std::sin(azimuth) * 0.5f + 0.5f;
        const float t = -std::cos(azimuth) * 0.5f + 0.5f;

        verts.emplace_back(s, t,  x, y, z,  x * r, y * r, z * r);
    }

    int numVerts = 0;
    for (int e = 1; e <= _elevationSteps - 1; e++) {
        const float de = static_cast<float>(e) / static_cast<float>(_elevationSteps);
        const float elevation = glm::radians(lift + de * (90.f - lift));

        const float y = std::sin(elevation);

        for (int a = 0; a < _azimuthSteps; a++) {
            const float azimuth = glm::radians((a * 360.f) / _azimuthSteps);

            const float x = std::cos(elevation) * std::sin(azimuth);
            const float z = -std::cos(elevation) * std::cos(azimuth);

            float s = (static_cast<float>(_elevationSteps - e) /
                       static_cast<float>(_elevationSteps))* std::sin(azimuth);
            float t = (static_cast<float>(_elevationSteps - e) /
                       static_cast<float>(_elevationSteps)) * -std::cos(azimuth);
            s = s * 0.5f + 0.5f;
            t = t * 0.5f + 0.5f;

            verts.emplace_back(s, t,  x, y, z,  x * r, y * r, z * r);

            indices.push_back(numVerts);
            indices.push_back(_azimuthSteps + numVerts);
            numVerts++;
        }

        indices.push_back(numVerts - _azimuthSteps);
        indices.push_back(numVerts);
    }

    const int e = _elevationSteps;
    const float de = static_cast<float>(e) / static_cast<float>(_elevationSteps);
    const float elevation = glm::radians(lift + de * (90.f - lift));
    const float y = std::sin(elevation);
    verts.push_back({ 0.5f, 0.5f,  0.f, 1.f, 0.f,  0.f, y * r, 0.f });

    glNamedBufferStorage(
        _vbo,
        verts.size() * sizeof(VertexData),
        verts.data(),
        GL_NONE_BIT
    );

    indices.push_back(numVerts + _azimuthSteps);
    for (int a = 1; a <= _azimuthSteps; a++) {
        indices.push_back(numVerts + _azimuthSteps - a);
    }
    indices.push_back(numVerts + _azimuthSteps - 1);

    glNamedBufferStorage(
        _ibo,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_NONE_BIT
    );
}

Dome::~Dome() {
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ibo);
    glDeleteVertexArrays(1, &_vao);
}

void Dome::draw() const {
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
