/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/utils/box.h>

#include <sgct/opengl.h>
#include <array>

namespace sgct::utils {

Box::Box(float size, TextureMappingMode mode) {
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


    const float halfSize = size / 2.f;
    std::array<VertexData, 36> v;

    if (mode == TextureMappingMode::Regular) {
        // A (front/+z)
        v[0] = { 0.f, 1.f, 0.f, 0.f, 1.f, -halfSize,  halfSize, halfSize };
        v[1] = { 0.f, 0.f, 0.f, 0.f, 1.f, -halfSize, -halfSize, halfSize };
        v[2] = { 1.f, 0.f, 0.f, 0.f, 1.f,  halfSize, -halfSize, halfSize };
        v[3] = { 0.f, 1.f, 0.f, 0.f, 1.f, -halfSize,  halfSize, halfSize };
        v[4] = { 1.f, 0.f, 0.f, 0.f, 1.f,  halfSize, -halfSize, halfSize };
        v[5] = { 1.f, 1.f, 0.f, 0.f, 1.f,  halfSize,  halfSize, halfSize };

        // B (right/+x)
        v[6] = { 0.f, 1.f, 1.f, 0.f, 0.f, halfSize,  halfSize,  halfSize };
        v[7] = { 0.f, 0.f, 1.f, 0.f, 0.f, halfSize, -halfSize,  halfSize };
        v[8] = { 1.f, 0.f, 1.f, 0.f, 0.f, halfSize, -halfSize, -halfSize };
        v[9] = { 0.f, 1.f, 1.f, 0.f, 0.f, halfSize,  halfSize,  halfSize };
        v[10] = { 1.f, 0.f, 1.f, 0.f, 0.f, halfSize, -halfSize, -halfSize };
        v[11] = { 1.f, 1.f, 1.f, 0.f, 0.f, halfSize,  halfSize, -halfSize };

        // C (Back/-z)
        v[12] = { 0.f, 1.f, 0.f, 0.f, -1.f,  halfSize,  halfSize, -halfSize };
        v[13] = { 0.f, 0.f, 0.f, 0.f, -1.f,  halfSize, -halfSize, -halfSize };
        v[14] = { 1.f, 0.f, 0.f, 0.f, -1.f, -halfSize, -halfSize, -halfSize };
        v[15] = { 0.f, 1.f, 0.f, 0.f, -1.f,  halfSize,  halfSize, -halfSize };
        v[16] = { 1.f, 0.f, 0.f, 0.f, -1.f, -halfSize, -halfSize, -halfSize };
        v[17] = { 1.f, 1.f, 0.f, 0.f, -1.f, -halfSize,  halfSize, -halfSize };

        // D (Left/-x)
        v[18] = { 0.f, 1.f, -1.f, 0.f, 0.f, -halfSize,  halfSize, -halfSize };
        v[19] = { 0.f, 0.f, -1.f, 0.f, 0.f, -halfSize, -halfSize, -halfSize };
        v[20] = { 1.f, 0.f, -1.f, 0.f, 0.f, -halfSize, -halfSize,  halfSize };
        v[21] = { 0.f, 1.f, -1.f, 0.f, 0.f, -halfSize,  halfSize, -halfSize };
        v[22] = { 1.f, 0.f, -1.f, 0.f, 0.f, -halfSize, -halfSize,  halfSize };
        v[23] = { 1.f, 1.f, -1.f, 0.f, 0.f, -halfSize,  halfSize,  halfSize };

        // E (Top/+y)
        v[24] = { 0.f, 1.f, 0.f, 1.f, 0.f, -halfSize, halfSize, -halfSize };
        v[25] = { 0.f, 0.f, 0.f, 1.f, 0.f, -halfSize, halfSize,  halfSize };
        v[26] = { 1.f, 0.f, 0.f, 1.f, 0.f,  halfSize, halfSize,  halfSize };
        v[27] = { 0.f, 1.f, 0.f, 1.f, 0.f, -halfSize, halfSize, -halfSize };
        v[28] = { 1.f, 0.f, 0.f, 1.f, 0.f,  halfSize, halfSize,  halfSize };
        v[29] = { 1.f, 1.f, 0.f, 1.f, 0.f,  halfSize, halfSize, -halfSize };

        // F (Bottom/-y)
        v[30] = { 0.f, 1.f, 0.f, -1.f, 0.f, -halfSize, -halfSize,  halfSize };
        v[31] = { 0.f, 0.f, 0.f, -1.f, 0.f, -halfSize, -halfSize, -halfSize };
        v[32] = { 1.f, 0.f, 0.f, -1.f, 0.f,  halfSize, -halfSize, -halfSize };
        v[33] = { 0.f, 1.f, 0.f, -1.f, 0.f, -halfSize, -halfSize,  halfSize };
        v[34] = { 1.f, 0.f, 0.f, -1.f, 0.f,  halfSize, -halfSize, -halfSize };
        v[35] = { 1.f, 1.f, 0.f, -1.f, 0.f,  halfSize, -halfSize,  halfSize };
    }
    else if (mode == TextureMappingMode::CubeMap) {
        // A (front/+z)
        v[0] = { 0.f, 1.f, 0.f, 0.f, 1.f, -halfSize, halfSize, halfSize };
        v[1] = { 0.f, 0.5f, 0.f, 0.f, 1.f, -halfSize, -halfSize, halfSize };
        v[2] = { 0.333333f, 0.5f, 0.f, 0.f, 1.f, halfSize, -halfSize, halfSize };
        v[3] = { 0.f, 1.f, 0.f, 0.f, 1.f, -halfSize, halfSize, halfSize };
        v[4] = { 0.333333f, 0.5f, 0.f, 0.f, 1.f, halfSize, -halfSize, halfSize };
        v[5] = { 0.333333f, 1.f, 0.f, 0.f, 1.f, halfSize, halfSize, halfSize };

        // B (right/+x)
        v[6] = { 0.333334f, 1.f, 1.f, 0.f, 0.f, halfSize, halfSize, halfSize };
        v[7] = { 0.333334f, 0.5f, 1.f, 0.f, 0.f, halfSize, -halfSize, halfSize };
        v[8] = { 0.666666f, 0.5f, 1.f, 0.f, 0.f, halfSize, -halfSize, -halfSize };
        v[9] = { 0.333334f, 1.f, 1.f, 0.f, 0.f, halfSize, halfSize, halfSize };
        v[10] = { 0.666666f, 0.5f, 1.f, 0.f, 0.f, halfSize, -halfSize, -halfSize };
        v[11] = { 0.666666f, 1.f, 1.f, 0.f, 0.f, halfSize, halfSize, -halfSize };

        // C (Back/-z)
        v[12] = { 0.666667f, 1.f, 0.f, 0.f, -1.f, halfSize, halfSize, -halfSize };
        v[13] = { 0.666667f, 0.5f, 0.f, 0.f, -1.f, halfSize, -halfSize, -halfSize };
        v[14] = { 1.f, 0.5f, 0.f, 0.f, -1.f, -halfSize, -halfSize, -halfSize };
        v[15] = { 0.666667f, 1.f, 0.f, 0.f, -1.f, halfSize, halfSize, -halfSize };
        v[16] = { 1.f, 0.5f, 0.f, 0.f, -1.f, -halfSize, -halfSize, -halfSize };
        v[17] = { 1.f, 1.f, 0.f, 0.f, -1.f, -halfSize, halfSize, -halfSize };

        // D (Left/-x)
        v[18] = { 0.f, 0.5f, -1.f, 0.f, 0.f, -halfSize, halfSize, -halfSize };
        v[19] = { 0.f, 0.f, -1.f, 0.f, 0.f, -halfSize, -halfSize, -halfSize };
        v[20] = { 0.333333f, 0.f, -1.f, 0.f, 0.f, -halfSize, -halfSize, halfSize };
        v[21] = { 0.f, 0.5f, -1.f, 0.f, 0.f, -halfSize, halfSize, -halfSize };
        v[22] = { 0.333333f, 0.f, -1.f, 0.f, 0.f, -halfSize, -halfSize, halfSize };
        v[23] = { 0.333333f, 0.5f, -1.f, 0.f, 0.f, -halfSize, halfSize, halfSize };

        // E (Top/+y)
        v[24] = { 0.333334f, 0.5f, 0.f, 1.f, 0.f, -halfSize, halfSize, -halfSize };
        v[25] = { 0.333334f, 0.f, 0.f, 1.f, 0.f, -halfSize, halfSize, halfSize };
        v[26] = { 0.666666f, 0.f, 0.f, 1.f, 0.f, halfSize, halfSize, halfSize };
        v[27] = { 0.333334f, 0.5f, 0.f, 1.f, 0.f, -halfSize, halfSize, -halfSize };
        v[28] = { 0.666666f, 0.f, 0.f, 1.f, 0.f, halfSize, halfSize, halfSize };
        v[29] = { 0.666666f, 0.5f, 0.f, 1.f, 0.f, halfSize, halfSize, -halfSize };

        // F (Bottom/-y)
        v[30] = { 0.666667f, 0.5f, 0.f, -1.f, 0.f, -halfSize, -halfSize, halfSize };
        v[31] = { 0.666667f, 0.f, 0.f, -1.f, 0.f, -halfSize, -halfSize, -halfSize };
        v[32] = { 1.f, 0.f, 0.f, -1.f, 0.f, halfSize, -halfSize, -halfSize };
        v[33] = { 0.666667f, 0.5f, 0.f, -1.f, 0.f, -halfSize, -halfSize, halfSize };
        v[34] = { 1.f, 0.f, 0.f, -1.f, 0.f, halfSize, -halfSize, -halfSize };
        v[35] = { 1.f, 0.5f, 0.f, -1.f, 0.f, halfSize, -halfSize, halfSize };
    }
    else { // skybox
        // A (front/+z)
        v[0] = { 1.f, 0.666666f, 0.f, 0.f, 1.f, -halfSize, halfSize, halfSize };
        v[1] = { 1.f, 0.333334f, 0.f, 0.f, 1.f, -halfSize, -halfSize, halfSize };
        v[2] = { 0.751f, 0.333334f, 0.f, 0.f, 1.f, halfSize, -halfSize, halfSize };
        v[3] = { 1.f, 0.666666f, 0.f, 0.f, 1.f, -halfSize, halfSize, halfSize };
        v[4] = { 0.751f, 0.333334f, 0.f, 0.f, 1.f, halfSize, -halfSize, halfSize };
        v[5] = { 0.751f, 0.666666f, 0.f, 0.f, 1.f, halfSize, halfSize, halfSize };

        // B (right/+x)
        v[6] = { 0.749f, 0.666666f, 1.f, 0.f, 0.f, halfSize, halfSize, halfSize };
        v[7] = { 0.749f, 0.333334f, 1.f, 0.f, 0.f, halfSize, -halfSize, halfSize };
        v[8] = { 0.501f, 0.333334f, 1.f, 0.f, 0.f, halfSize, -halfSize, -halfSize };
        v[9] = { 0.749f, 0.666666f, 1.f, 0.f, 0.f, halfSize, halfSize, halfSize };
        v[10] = { 0.501f, 0.333334f, 1.f, 0.f, 0.f, halfSize, -halfSize, -halfSize };
        v[11] = { 0.501f, 0.666666f, 1.f, 0.f, 0.f, halfSize, halfSize, -halfSize };

        // C (Back/-z)
        v[12] = { 0.499f, 0.666666f, 0.f, 0.f, -1.f,  halfSize,  halfSize, -halfSize };
        v[13] = { 0.499f, 0.333334f, 0.f, 0.f, -1.f,  halfSize, -halfSize, -halfSize };
        v[14] = { 0.251f, 0.333334f, 0.f, 0.f, -1.f, -halfSize, -halfSize, -halfSize };
        v[15] = { 0.499f, 0.666666f, 0.f, 0.f, -1.f,  halfSize,  halfSize, -halfSize };
        v[16] = { 0.251f, 0.333334f, 0.f, 0.f, -1.f, -halfSize, -halfSize, -halfSize };
        v[17] = { 0.251f, 0.666666f, 0.f, 0.f, -1.f, -halfSize,  halfSize, -halfSize };

        // D (Left/-x)
        v[18] = { 0.249f, 0.666666f, -1.f, 0.f, 0.f, -halfSize, halfSize, -halfSize };
        v[19] = { 0.249f, 0.333334f, -1.f, 0.f, 0.f, -halfSize, -halfSize, -halfSize };
        v[20] = { 0.000f, 0.333334f, -1.f, 0.f, 0.f, -halfSize, -halfSize, halfSize };
        v[21] = { 0.249f, 0.666666f, -1.f, 0.f, 0.f, -halfSize, halfSize, -halfSize };
        v[22] = { 0.000f, 0.333334f, -1.f, 0.f, 0.f, -halfSize, -halfSize, halfSize };
        v[23] = { 0.000f, 0.666666f, -1.f, 0.f, 0.f, -halfSize, halfSize, halfSize };

        // E (Top/+y)
        v[24] = { 0.251f, 0.666667f, 0.f, 1.f, 0.f, -halfSize, halfSize, -halfSize };
        v[25] = { 0.251f, 1.f, 0.f, 1.f, 0.f, -halfSize, halfSize,  halfSize };
        v[26] = { 0.499f, 1.f, 0.f, 1.f, 0.f,  halfSize, halfSize,  halfSize };
        v[27] = { 0.251f, 0.666667f, 0.f, 1.f, 0.f, -halfSize, halfSize, -halfSize };
        v[28] = { 0.499f, 1.f, 0.f, 1.f, 0.f,  halfSize, halfSize,  halfSize };
        v[29] = { 0.499f, 0.666667f, 0.f, 1.f, 0.f,  halfSize, halfSize, -halfSize };

        // F (Bottom/-y)
        v[30] = { 0.251f, 0.f, 0.f, -1.f, 0.f, -halfSize, -halfSize,  halfSize };
        v[31] = { 0.251f, 0.333333f, 0.f, -1.f, 0.f, -halfSize, -halfSize, -halfSize };
        v[32] = { 0.499f, 0.333333f, 0.f, -1.f, 0.f,  halfSize, -halfSize, -halfSize };
        v[33] = { 0.251f, 0.f, 0.f, -1.f, 0.f, -halfSize, -halfSize,  halfSize };
        v[34] = { 0.499f, 0.333333f, 0.f, -1.f, 0.f,  halfSize, -halfSize, -halfSize };
        v[35] = { 0.499f, 0.f, 0.f, -1.f, 0.f,  halfSize, -halfSize,  halfSize };
    }

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    constexpr int s = sizeof(VertexData);
    glBufferData(GL_ARRAY_BUFFER, v.size() * s, v.data(), GL_STATIC_DRAW);

    // texcoords
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, s, nullptr);

    // normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, s, reinterpret_cast<void*>(8));

    // vert positions
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, s, reinterpret_cast<void*>(20));

    glBindVertexArray(0);
}

Box::~Box() {
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
}

void Box::draw() {
    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

} // namespace sgct::utils
