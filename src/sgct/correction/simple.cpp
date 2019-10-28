/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/correction/simple.h>

namespace sgct::core::correction {

Buffer setupMaskMesh(const glm::ivec2& pos, const glm::ivec2& size) {
    Buffer buff;
    buff.indices = { 0, 3, 1, 2 };

    buff.vertices.resize(4);
    buff.vertices[0].r = 1.f;
    buff.vertices[0].g = 1.f;
    buff.vertices[0].b = 1.f;
    buff.vertices[0].a = 1.f;
    buff.vertices[0].s = 0.f;
    buff.vertices[0].t = 0.f;
    buff.vertices[0].x = 2.f * pos.x - 1.f;
    buff.vertices[0].y = 2.f * pos.y - 1.f;

    buff.vertices[1].r = 1.f;
    buff.vertices[1].g = 1.f;
    buff.vertices[1].b = 1.f;
    buff.vertices[1].a = 1.f;
    buff.vertices[1].s = 1.f;
    buff.vertices[1].t = 0.f;
    buff.vertices[1].x = 2.f * (pos.x + size.x) - 1.f;
    buff.vertices[1].y = 2.f * pos.y - 1.f;

    buff.vertices[2].r = 1.f;
    buff.vertices[2].g = 1.f;
    buff.vertices[2].b = 1.f;
    buff.vertices[2].a = 1.f;
    buff.vertices[2].s = 1.f;
    buff.vertices[2].t = 1.f;
    buff.vertices[2].x = 2.f * (pos.x + size.x) - 1.f;
    buff.vertices[2].y = 2.f * (pos.y + size.y) - 1.f;

    buff.vertices[3].r = 1.f;
    buff.vertices[3].g = 1.f;
    buff.vertices[3].b = 1.f;
    buff.vertices[3].a = 1.f;
    buff.vertices[3].s = 0.f;
    buff.vertices[3].t = 1.f;
    buff.vertices[3].x = 2.f * pos.x - 1.f;
    buff.vertices[3].y = 2.f * (pos.y + size.y) - 1.f;

    buff.geometryType = GL_TRIANGLE_STRIP;

    return buff;
}

Buffer setupSimpleMesh(const glm::ivec2& pos, const glm::ivec2& size) {
    Buffer buff;
    buff.indices = { 0, 3, 1, 2 };

    buff.vertices.resize(4);
    buff.vertices[0].r = 1.f;
    buff.vertices[0].g = 1.f;
    buff.vertices[0].b = 1.f;
    buff.vertices[0].a = 1.f;
    buff.vertices[0].s = static_cast<float>(pos.x);
    buff.vertices[0].t = static_cast<float>(pos.y);
    buff.vertices[0].x = 2.f * pos.x - 1.f;
    buff.vertices[0].y = 2.f * pos.y - 1.f;

    buff.vertices[1].r = 1.f;
    buff.vertices[1].g = 1.f;
    buff.vertices[1].b = 1.f;
    buff.vertices[1].a = 1.f;
    buff.vertices[1].s = static_cast<float>(pos.x + size.x);
    buff.vertices[1].t = static_cast<float>(pos.y);
    buff.vertices[1].x = 2.f * (pos.x + size.x) - 1.f;
    buff.vertices[1].y = 2.f * pos.y - 1.f;

    buff.vertices[2].r = 1.f;
    buff.vertices[2].g = 1.f;
    buff.vertices[2].b = 1.f;
    buff.vertices[2].a = 1.f;
    buff.vertices[2].s = 1.f * size.x + pos.x;
    buff.vertices[2].t = 1.f * size.y + pos.y;
    buff.vertices[2].x = 2.f * (pos.x + size.x) - 1.f;
    buff.vertices[2].y = 2.f * (pos.y + size.y) - 1.f;

    buff.vertices[3].r = 1.f;
    buff.vertices[3].g = 1.f;
    buff.vertices[3].b = 1.f;
    buff.vertices[3].a = 1.f;
    buff.vertices[3].s = static_cast<float>(pos.x);
    buff.vertices[3].t = static_cast<float>(pos.y + size.y);
    buff.vertices[3].x = 2.f * pos.x - 1.f;
    buff.vertices[3].y = 2.f * (pos.y + size.y) - 1.f;

    buff.geometryType = GL_TRIANGLE_STRIP;

    return buff;
}

} // namespace sgct::core::correction
