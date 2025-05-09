/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
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
        float t = 0.f;  // Texcoord0 -> size=8
        float nx = 0.f;
        float ny = 0.f;
        float nz = 0.f; // size=12
        float x = 0.f;
        float y = 0.f;
        float z = 0.f;  // size=12 ; total size=32 = power of two
    };

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
            ++numVerts;
        }

        indices.push_back(numVerts - _azimuthSteps);
        indices.push_back(numVerts);
    }

    const int e = _elevationSteps;
    const float de = static_cast<float>(e) / static_cast<float>(_elevationSteps);
    const float elevation = glm::radians(lift + de * (90.f - lift));
    const float y = std::sin(elevation);
    verts.push_back({ 0.5f, 0.5f,  0.f, 1.f, 0.f,  0.f, y * r, 0.f });

    indices.push_back(numVerts + _azimuthSteps);
    for (int a = 1; a <= _azimuthSteps; a++) {
        indices.push_back(numVerts + _azimuthSteps - a);
    }
    indices.push_back(numVerts + _azimuthSteps - 1);

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    const GLsizei size = sizeof(VertexData);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * size, verts.data(), GL_STATIC_DRAW);

    // texcoords
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, size, nullptr);
    // normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, size, reinterpret_cast<void*>(8));
    // vert positions
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, size, reinterpret_cast<void*>(20));

    glGenBuffers(1, &_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW
    );

    glBindVertexArray(0);
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
