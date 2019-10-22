/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/statistics.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/helpers/stringfunctions.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <string>

#undef min
#undef max

namespace {
    constexpr const char* StatsVertShader = R"(
#version 330 core

layout (location = 0) in vec2 vertPosition;

uniform mat4 MVP;

void main() {
    gl_Position = MVP * vec4(vertPosition, 0.0, 1.0);
}
)";

    constexpr const char* StatsFragShader = R"(
#version 330 core

uniform vec4 Col;
out vec4 Color;

void main() {
    Color = Col;
}
)";

} // namespace

namespace sgct::core {

Statistics::Statistics() {
    for (unsigned int i = 0; i < StatsHistoryLength; i++) {
        _dynamicVertexList.frameTime[i].x = static_cast<float>(i);
        _dynamicVertexList.frameTime[i].y = 0.f;
        _dynamicVertexList.drawTime[i].x = static_cast<float>(i);
        _dynamicVertexList.drawTime[i].y = 0.f;
        _dynamicVertexList.syncTime[i].x = static_cast<float>(i);
        _dynamicVertexList.syncTime[i].y = 0.f;
        _dynamicVertexList.loopTimeMax[i].x = static_cast<float>(i);
        _dynamicVertexList.loopTimeMax[i].y = 0.f;
        _dynamicVertexList.loopTimeMin[i].x = static_cast<float>(i);
        _dynamicVertexList.loopTimeMin[i].y = 0.f;
    }
}

Statistics::~Statistics() {
    _shader.deleteProgram();
    
    glDeleteBuffers(2, &_dynamicVBO[0]);
    glDeleteVertexArrays(2, &_dynamicVAO[0]);
    glDeleteBuffers(1, &_staticVBO);
    glDeleteBuffers(1, &_staticVAO);
}

void Statistics::initVBO() {
    // STATIC BACKGROUND QUAD
    _staticVerts.push_back(0.f);
    _staticVerts.push_back(0.f);
    _staticVerts.push_back(static_cast<float>(StatsHistoryLength * 2));
    _staticVerts.push_back(0.f);
    _staticVerts.push_back(0.f);
    _staticVerts.push_back(1.f / 30.f);
    _staticVerts.push_back(static_cast<float>(StatsHistoryLength * 2));
    _staticVerts.push_back(1.f / 30.f);
    
    // STATIC 1 ms LINES
    _nLines = 0;
    for (float f = 0.001f; f < (1.f / 30.f); f += 0.001f) {
        _staticVerts.push_back(0.f);
        _staticVerts.push_back(f);
        _staticVerts.push_back(static_cast<float>(StatsHistoryLength * 2));
        _staticVerts.push_back(f);

        _nLines++;
    }

    // STATIC 0, 30 & 60 FPS LINES
    _staticVerts.push_back(0.f);
    _staticVerts.push_back(0.f);
    _staticVerts.push_back(static_cast<float>(StatsHistoryLength * 2));
    _staticVerts.push_back(0.f);

    _staticVerts.push_back(0.f);
    _staticVerts.push_back(1.f / 30.f);
    _staticVerts.push_back(static_cast<float>(StatsHistoryLength * 2));
    _staticVerts.push_back(1.f / 30.f);

    _staticVerts.push_back(0.f);
    _staticVerts.push_back(1.f / 60.f);
    _staticVerts.push_back(static_cast<float>(StatsHistoryLength * 2));
    _staticVerts.push_back(1.f / 60.f);

    glGenVertexArrays(2, &_dynamicVAO[0]);
    glGenVertexArrays(1, &_staticVAO);

    MessageHandler::printDebug("Statistics: Generating VAOs");
    for (unsigned int i = 0; i < 2; i++) {
        MessageHandler::printDebug("\t%d", _dynamicVAO[i]);
    }
    MessageHandler::printDebug("\t%d", _staticVAO);

    glGenBuffers(2, &_dynamicVBO[0]);
    glGenBuffers(1, &_staticVBO);

    MessageHandler::printDebug("Statistics: Generating VBOs");
    for (unsigned int i = 0; i < 2; i++) {
        MessageHandler::printDebug("\t%d", _dynamicVBO[i]);
    }
    MessageHandler::printDebug("\t%d", _staticVBO);

    // double buffered VBOs
    for (unsigned int i = 0; i < 2; i++) {
        glBindVertexArray(_dynamicVAO[i]);
        glBindBuffer(GL_ARRAY_BUFFER, _dynamicVBO[i]);
        glBufferData(
            GL_ARRAY_BUFFER,
            StatsHistoryLength * sizeof(StatsVertex) * StatsNumberOfDynamicObjs,
            &_dynamicVertexList,
            GL_STREAM_DRAW
        );

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    glBindVertexArray(_staticVAO);
    glBindBuffer(GL_ARRAY_BUFFER, _staticVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        _staticVerts.size() * sizeof(float),
        _staticVerts.data(),
        GL_STATIC_DRAW
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    
    // SETUP SHADERS
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    _shader = ShaderProgram("StatsShader");
    _shader.addShaderSource(StatsVertShader, StatsFragShader);
    _shader.createAndLinkProgram();
    _shader.bind();

    _mvpLoc = _shader.getUniformLocation("MVP");
    _colorLoc = _shader.getUniformLocation("Col");

    _shader.unbind();
}

void Statistics::setAvgFPS(float afps) {
    _avgFPS = afps;
}

void Statistics::setFrameTime(float t) {
    _avgFrameTime = 0.f;
    _maxFrameTime = 0.f;
    _minFrameTime = std::numeric_limits<float>::max();

    for (int i = StatsHistoryLength - 2; i >= 0; i--) {
        _dynamicVertexList.frameTime[i + 1].y = _dynamicVertexList.frameTime[i].y;
        if (i < (StatsAverageLength - 1)) {
            _avgFrameTime += _dynamicVertexList.frameTime[i].y;
            _maxFrameTime = glm::max(_maxFrameTime, _dynamicVertexList.frameTime[i].y);
            _minFrameTime = glm::min(_minFrameTime, _dynamicVertexList.frameTime[i].y);
        }
    }
    _dynamicVertexList.frameTime[0].y = t;
    
    _avgFrameTime += _dynamicVertexList.frameTime[0].y;
    _maxFrameTime = glm::max(_maxFrameTime, _dynamicVertexList.frameTime[0].y);
    _minFrameTime = glm::min(_minFrameTime, _dynamicVertexList.frameTime[0].y);

    _avgFrameTime /= static_cast<float>(StatsAverageLength);

    float sumSquaredDeviation = 0.f;
    for (int i = 0; i < StatsAverageLength; ++i) {
        sumSquaredDeviation += std::pow(
            _dynamicVertexList.frameTime[i].y - _avgFrameTime,
            2.f
        );
    }

    _stdDevFrameTime = glm::sqrt(
        sumSquaredDeviation / static_cast<float>(StatsAverageLength)
    );
}

void Statistics::setDrawTime(float t) {
    _avgDrawTime = 0.f;

    for (int i = StatsHistoryLength - 2; i >= 0; i--) {
        _dynamicVertexList.drawTime[i + 1].y = _dynamicVertexList.drawTime[i].y;
        if (i < (StatsAverageLength - 1)) {
            _avgDrawTime += _dynamicVertexList.drawTime[i].y;
        }
    }
    _dynamicVertexList.drawTime[0].y = t;
    
    _avgDrawTime += _dynamicVertexList.drawTime[0].y;
    _avgDrawTime /= static_cast<float>(StatsAverageLength);
}

void Statistics::setSyncTime(float t) {
    _avgSyncTime = 0.f;
    
    for (int i = StatsHistoryLength - 2; i >= 0; i--) {
        _dynamicVertexList.syncTime[i + 1].y = _dynamicVertexList.syncTime[i].y;
        if (i < (StatsAverageLength - 1)) {
            _avgSyncTime += _dynamicVertexList.syncTime[i].y;
        }
    }
    _dynamicVertexList.syncTime[0].y = t;
    
    _avgSyncTime += _dynamicVertexList.syncTime[0].y;
    _avgSyncTime /= static_cast<float>(StatsAverageLength);
}

void Statistics::setLoopTime(float min, float max) {
    std::rotate(
        std::rbegin(_dynamicVertexList.loopTimeMax),
        std::rbegin(_dynamicVertexList.loopTimeMax) + 1,
        std::rend(_dynamicVertexList.loopTimeMax)
    );
    _dynamicVertexList.loopTimeMax[0].y = max;
    
    std::rotate(
        std::rbegin(_dynamicVertexList.loopTimeMin),
        std::rbegin(_dynamicVertexList.loopTimeMin) + 1,
        std::rend(_dynamicVertexList.loopTimeMin)
    );
    _dynamicVertexList.loopTimeMin[0].y = min;
}

void Statistics::addSyncTime(float t) {
    _dynamicVertexList.syncTime[StatsHistoryLength - 1].y += t;
    _avgSyncTime += (t / static_cast<float>(StatsAverageLength));
}

void Statistics::update() {
    _vboIndex = 1 - _vboIndex; //ping-pong
    glBindBuffer(GL_ARRAY_BUFFER, _dynamicVBO[_vboIndex]);
    size_t size = StatsHistoryLength * sizeof(StatsVertex) * StatsNumberOfDynamicObjs;

    GLvoid* positionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    
    if (positionBuffer) {
        memcpy(positionBuffer, &_dynamicVertexList, size);
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Statistics::draw(float lineWidth) {
    _shader.bind();

    // gives an opengl error in mac os x (intel iris)
    glLineWidth(lineWidth);
        
    float size = static_cast<float>(StatsHistoryLength);
        
    glm::mat4 orthoMat = glm::ortho(0.f, size, 0.f, size);
    orthoMat = glm::translate(orthoMat, glm::vec3(0.f, size / 4.f, 0.f));
    orthoMat = glm::scale(orthoMat, glm::vec3(1.f, VertScale, 1.f));
        
    glUniformMatrix4fv(_mvpLoc, 1, GL_FALSE, &orthoMat[0][0]);
        
    glBindVertexArray(_staticVAO);

    // draw background (1024x1024 canvas)
    glUniform4fv(_colorLoc, 1, glm::value_ptr(_staticColorBackground));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // 1 ms lines
    glUniform4fv(_colorLoc, 1, glm::value_ptr(_staticColorGrid));
    glDrawArrays(GL_LINES, 4, _nLines * 2);
        
    // zero line, 60hz & 30hz
    glUniform4fv(_colorLoc, 1, glm::value_ptr(_staticColorFrequency));
    glDrawArrays(GL_LINES, 4 + _nLines * 2, 6);
        
    glBindVertexArray(_dynamicVAO[_vboIndex]);

    // frametime
    glUniform4fv(_colorLoc, 1, glm::value_ptr(_dynamicColors.frameTime));
    glDrawArrays(GL_LINE_STRIP, 0 * StatsHistoryLength, StatsHistoryLength);

    // drawtime
    glUniform4fv(_colorLoc, 1, glm::value_ptr(_dynamicColors.drawTime));
    glDrawArrays(GL_LINE_STRIP, 1 * StatsHistoryLength, StatsHistoryLength);

    // synctime
    glUniform4fv(_colorLoc, 1, glm::value_ptr(_dynamicColors.syncTime));
    glDrawArrays(GL_LINE_STRIP, 2 * StatsHistoryLength, StatsHistoryLength);

    // looptimemax
    glUniform4fv(_colorLoc, 1, glm::value_ptr(_dynamicColors.loopTimeMax));
    glDrawArrays(GL_LINE_STRIP, 3 * StatsHistoryLength, StatsHistoryLength);

    // looptimemin
    glUniform4fv(_colorLoc, 1, glm::value_ptr(_dynamicColors.loopTimeMin));
    glDrawArrays(GL_LINE_STRIP, 4 * StatsHistoryLength, StatsHistoryLength);

    glBindVertexArray(0);
    _shader.unbind();
}

float Statistics::getAvgFPS() const {
    return _avgFPS;
}

float Statistics::getAvgDrawTime() const {
    return _avgDrawTime;
}

float Statistics::getAvgSyncTime() const {
    return _avgSyncTime;
}

float Statistics::getAvgFrameTime() const {
    return _avgFrameTime;
}

float Statistics::getMinFrameTime() const {
    return _minFrameTime;
}

float Statistics::getMaxFrameTime() const {
    return _maxFrameTime;
}

float Statistics::getFrameTimeStandardDeviation() const {
    return _stdDevFrameTime;
}

float Statistics::getFrameTime() const {
    return _dynamicVertexList.frameTime[0].y;
}

float Statistics::getDrawTime() const {
    return _dynamicVertexList.drawTime[0].y;
}

float Statistics::getSyncTime() const {
    return _dynamicVertexList.syncTime[0].y;
}

} // namespace sgct::core
