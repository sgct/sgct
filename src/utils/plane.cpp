/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/utils/plane.h>

#include <sgct/opengl.h>
#include <array>

namespace sgct::utils {

Plane::Plane(float width, float height) {
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

    const std::array<VertexData, 4> verts = {
        VertexData{ 0.f, 0.f, 0.f, 0.f, 1.f, -width / 2.f, -height / 2.f, 0.f },
        VertexData{ 1.f, 0.f, 0.f, 0.f, 1.f,  width / 2.f, -height / 2.f, 0.f },
        VertexData{ 0.f, 1.f, 0.f, 0.f, 1.f, -width / 2.f,  height / 2.f, 0.f },
        VertexData{ 1.f, 1.f, 0.f, 0.f, 1.f,  width / 2.f,  height / 2.f, 0.f }
    };
    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    const GLsizei size = sizeof(VertexData);
    glBufferData(GL_ARRAY_BUFFER, 4 * size, verts.data(), GL_STATIC_DRAW);

    // texcoords
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, size, nullptr);

    // normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, size, reinterpret_cast<void*>(8));

    // vert positions
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, size, reinterpret_cast<void*>(20));

    glBindVertexArray(0);
}

Plane::~Plane() {
    glDeleteBuffers(1, &_vbo);
    glDeleteVertexArrays(1, &_vao);
}

void Plane::draw() {
    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

} // namespace sgct::utils
