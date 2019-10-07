/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/utils/domegrid.h>

#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/ogl_headers.h>
#include <glm/gtc/constants.hpp>

namespace sgct::utils {

DomeGrid::DomeGrid(float radius, float FOV, unsigned int segments, unsigned int rings,
                   unsigned int resolution)
    : _resolution(resolution)
    , _rings(rings)
    , _segments(segments)
{
    // must be four or higher
    if (_resolution < 4) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Warning: Dome geometry resolution must be higher than 4\n"
        );
        _resolution = 4;
    }

    createVBO(radius, FOV);

    // if error occured
    if (!Engine::checkForOGLErrors()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "SGCT Utils: Dome creation error\n"
        );
    }
}

DomeGrid::~DomeGrid() {
    glDeleteBuffers(1, &_vbo);
    glDeleteVertexArrays(1, &_vao);
}

void DomeGrid::draw() {
    drawVAO();
}

void DomeGrid::drawVBO() {
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_VERTEX_ARRAY);

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    glVertexPointer(3, GL_FLOAT, 0, nullptr);

    for (unsigned int r = 0; r < _rings; r++) {
        glDrawArrays(GL_LINE_LOOP, r * _resolution, _resolution);
    }
    for (unsigned int s = 0; s < _segments; s++) {
        glDrawArrays(
            GL_LINE_STRIP,
            _rings * _resolution + s * ((_resolution / 4) + 1),
            (_resolution / 4) + 1
        );
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPopClientAttrib();
}

void DomeGrid::drawVAO() {
    glBindVertexArray(_vao);

    for (unsigned int r = 0; r < _rings; r++) {
        glDrawArrays(GL_LINE_LOOP, r * _resolution, _resolution);
    }
    for (unsigned int s = 0; s < _segments; s++) {
        glDrawArrays(
            GL_LINE_STRIP,
            _rings * _resolution + s * ((_resolution / 4) + 1),
            (_resolution / 4) + 1
        );
    }

    glBindVertexArray(0);
}

void DomeGrid::createVBO(float radius, float FOV) {
    const unsigned int numberOfVertices = (_segments * ((_resolution / 4) + 1) +
                                           _rings * _resolution) * 6;
    std::vector<float> verts(numberOfVertices, 0.f);

    unsigned int pos = 0;

    // create rings
    for (unsigned int r = 1; r <= _rings; r++) {
        const float elevationAngle = glm::radians<float>(
            (FOV / 2.f) * (static_cast<float>(r) / static_cast<float>(_rings))
        );
        for (unsigned int i = 0; i < _resolution; i++) {
            const float theta = glm::two_pi<float>() *
                (static_cast<float>(i) / static_cast<float>(_resolution));

            verts[pos] = radius * sin(elevationAngle) * cos(theta);
            verts[pos + 1] = radius * cos(elevationAngle);
            verts[pos + 2] = radius * sin(elevationAngle) * sin(theta);
            pos += 3;
        }
    }

    // create segments
    for (unsigned int s = 0; s < _segments; s++) {
        const float theta = glm::two_pi<float>() *
            (static_cast<float>(s) / static_cast<float>(_segments));

        for (unsigned int i = 0; i < (_resolution / 4) + 1; i++) {
            const float elevationAngle = glm::radians<float>(FOV / 2.f) *
                (static_cast<float>(i) / static_cast<float>(_resolution / 4));

            verts[pos] = radius * sin(elevationAngle) * cos(theta);
            verts[pos + 1] = radius * cos(elevationAngle);
            verts[pos + 2] = radius * sin(elevationAngle) * sin(theta);
            pos += 3;
        }
    }

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);
    glEnableVertexAttribArray(0);
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "DomeGrid: Generating VAO: %d\n", _vao
    );

    glGenBuffers(1, &_vbo);
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "DomeGrid: Generating VBO: %d\n", _vbo
    );

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        verts.size() * sizeof(float),
        verts.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace sgct::utils
