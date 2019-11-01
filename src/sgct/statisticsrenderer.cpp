/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/statisticsrenderer.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {
    constexpr const float VerticalScale = 5000.f;
    const glm::vec4 ColorStaticGrid = glm::vec4(1.f, 1.f, 1.f, 0.2f);
    const glm::vec4 ColorStaticFrequency = glm::vec4(1.f, 0.f, 0.f, 1.f);
    const glm::vec4 ColorStaticBackground = glm::vec4(0.f, 0.f, 0.f, 0.5f);

    const glm::vec4 ColorFrameTime = glm::vec4(1.f, 1.f, 0.f, 0.8f);
    const glm::vec4 ColorDrawTime = glm::vec4(1.f, 0.f, 1.f, 0.8f);
    const glm::vec4 ColorSyncTime = glm::vec4(0.f, 1.f, 1.f, 0.8f);
    const glm::vec4 ColorLoopTimeMax = glm::vec4(0.4f, 0.4f, 1.f, 0.8f);
    const glm::vec4 ColorLoopTimeMin = glm::vec4(0.f, 0.f, 0.8f, 0.8f);

    constexpr const char* StatsVertShader = R"(
#version 330 core

layout (location = 0) in vec2 vertPosition;

uniform mat4 mvp;

void main() {
  gl_Position = mvp * vec4(vertPosition, 0.0, 1.0);
}
)";

    constexpr const char* StatsFragShader = R"(
#version 330 core

uniform vec4 col;
out vec4 color;

void main() { color = col; }
)";

} // namespace

namespace sgct::core {

StatisticsRenderer::StatisticsRenderer(const Engine::Statistics& statistics)
    : _statistics(statistics)
{
    // Static background quad
    std::vector<float> staticVerts;
    staticVerts.push_back(0.f);
    staticVerts.push_back(0.f);
    staticVerts.push_back(static_cast<float>(_statistics.HistoryLength));
    staticVerts.push_back(0.f);
    staticVerts.push_back(0.f);
    staticVerts.push_back(1.f / 30.f);
    staticVerts.push_back(static_cast<float>(_statistics.HistoryLength));
    staticVerts.push_back(1.f / 30.f);

    // Static 1 ms lines
    _static.nLines = 0;
    for (float f = 0.001f; f < (1.f / 30.f); f += 0.001f) {
        staticVerts.push_back(0.f);
        staticVerts.push_back(f);
        staticVerts.push_back(static_cast<float>(_statistics.HistoryLength));
        staticVerts.push_back(f);
        _static.nLines++;
    }

    // Static 0, 30 & 60 FPS lines
    staticVerts.push_back(0.f);
    staticVerts.push_back(0.f);
    staticVerts.push_back(static_cast<float>(_statistics.HistoryLength));
    staticVerts.push_back(0.f);

    staticVerts.push_back(0.f);
    staticVerts.push_back(1.f / 30.f);
    staticVerts.push_back(static_cast<float>(_statistics.HistoryLength));
    staticVerts.push_back(1.f / 30.f);

    staticVerts.push_back(0.f);
    staticVerts.push_back(1.f / 60.f);
    staticVerts.push_back(static_cast<float>(_statistics.HistoryLength));
    staticVerts.push_back(1.f / 60.f);

    glGenVertexArrays(1, &_static.vao);
    glGenBuffers(1, &_static.vbo);
    glBindVertexArray(_static.vao);
    glBindBuffer(GL_ARRAY_BUFFER, _static.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        staticVerts.size() * sizeof(float),
        staticVerts.data(),
        GL_STATIC_DRAW
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glGenVertexArrays(1, &_dynamic.vao);
    glGenBuffers(1, &_dynamic.vbo);
    glBindVertexArray(_dynamic.vao);
    glBindBuffer(GL_ARRAY_BUFFER, _dynamic.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        5 * _statistics.HistoryLength * sizeof(float),
        nullptr,
        GL_DYNAMIC_DRAW
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);

    // Setup shaders
    _shader = ShaderProgram("StaticStatsShader");
    _shader.addShaderSource(StatsVertShader, StatsFragShader);
    _shader.createAndLinkProgram();
    _shader.bind();
    _mvpLoc = _shader.getUniformLocation("mvp");
    _colorLoc = _shader.getUniformLocation("col");
    _shader.unbind();

    _vertexBuffer = std::make_unique<Vertices>();
}

StatisticsRenderer::~StatisticsRenderer() {
    glDeleteVertexArrays(1, &_static.vao);
    glDeleteBuffers(1, &_static.vbo);
    glDeleteVertexArrays(1, &_dynamic.vao);
    glDeleteBuffers(1, &_dynamic.vbo);
    _shader.deleteProgram();
}

void StatisticsRenderer::update() {
    // The statistics window stores the values as 1D double arrays, but we want them to be
    // in 2D float arrays

    for (size_t i = 0; i < Engine::Statistics::HistoryLength; ++i) {
        _vertexBuffer->frametimes[i].x = static_cast<float>(i);
        _vertexBuffer->frametimes[i].y = static_cast<float>(_statistics.frametimes[i]);
    }
    for (size_t i = 0; i < Engine::Statistics::HistoryLength; ++i) {
        _vertexBuffer->drawTimes[i].x = static_cast<float>(i);
        _vertexBuffer->drawTimes[i].y = static_cast<float>(_statistics.drawTimes[i]);
    }
    for (size_t i = 0; i < Engine::Statistics::HistoryLength; ++i) {
        _vertexBuffer->syncTimes[i].x = static_cast<float>(i);
        _vertexBuffer->syncTimes[i].y = static_cast<float>(_statistics.syncTimes[i]);
    }
    for (size_t i = 0; i < Engine::Statistics::HistoryLength; ++i) {
        _vertexBuffer->loopTimeMin[i].x = static_cast<float>(i);
        _vertexBuffer->loopTimeMin[i].y = static_cast<float>(_statistics.loopTimeMin[i]);
    }
    for (size_t i = 0; i < Engine::Statistics::HistoryLength; ++i) {
        _vertexBuffer->loopTimeMax[i].x = static_cast<float>(i);
        _vertexBuffer->loopTimeMax[i].y = static_cast<float>(_statistics.loopTimeMax[i]);
    }

    glBindBuffer(GL_ARRAY_BUFFER, _dynamic.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(Vertices),
        &_vertexBuffer->frametimes,
        GL_DYNAMIC_DRAW
    );
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void StatisticsRenderer::render() {
    _shader.bind();

    float size = static_cast<float>(_statistics.HistoryLength);

    glm::mat4 orthoMat = glm::ortho(0.f, size, 0.f, size);
    orthoMat = glm::translate(orthoMat, glm::vec3(0.f, size / 4.f, 0.f));
    orthoMat = glm::scale(orthoMat, glm::vec3(1.f, VerticalScale, 1.f));

    glUniformMatrix4fv(_mvpLoc, 1, GL_FALSE, glm::value_ptr(orthoMat));

    glBindVertexArray(_static.vao);

    // draw background (1024x1024 canvas)
    glUniform4fv(_colorLoc, 1, glm::value_ptr(ColorStaticBackground));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // 1 ms lines
    glUniform4fv(_colorLoc, 1, glm::value_ptr(ColorStaticGrid));
    glDrawArrays(GL_LINES, 4, _static.nLines * 2);

    // zero line, 60hz & 30hz
    glUniform4fv(_colorLoc, 1, glm::value_ptr(ColorStaticFrequency));
    glDrawArrays(GL_LINES, 4 + _static.nLines * 2, 6);

    glBindVertexArray(_dynamic.vao);

    // frametime
    constexpr const int StatsLength = Engine::Statistics::HistoryLength;
    glUniform4fv(_colorLoc, 1, glm::value_ptr(ColorFrameTime));
    glDrawArrays(GL_LINE_STRIP, 0 * StatsLength, StatsLength);

    // drawtime
    glUniform4fv(_colorLoc, 1, glm::value_ptr(ColorDrawTime));
    glDrawArrays(GL_LINE_STRIP, 1 * StatsLength, StatsLength);

    // synctime
    glUniform4fv(_colorLoc, 1, glm::value_ptr(ColorSyncTime));
    glDrawArrays(GL_LINE_STRIP, 2 * StatsLength, StatsLength);

    // looptimemax
    glUniform4fv(_colorLoc, 1, glm::value_ptr(ColorLoopTimeMax));
    glDrawArrays(GL_LINE_STRIP, 3 * StatsLength, StatsLength);

    // looptimemin
    glUniform4fv(_colorLoc, 1, glm::value_ptr(ColorLoopTimeMin));
    glDrawArrays(GL_LINE_STRIP, 4 * StatsLength, StatsLength);

    glBindVertexArray(0);
    _shader.unbind();
}

} // namespace sgct::core
