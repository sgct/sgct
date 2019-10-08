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
    constexpr const char* Stats_Vert_Shader = R"(
**glsl_version**

layout (location = 0) in vec2 Position;

uniform mat4 MVP;

void main() {
    gl_Position = MVP * vec4(Position, 0.0, 1.0);
};
)";

    constexpr const char* Stats_Frag_Shader = R"(
**glsl_version**

uniform vec4 Col;
out vec4 Color;

void main() {
    Color = Col;
}
)";

    constexpr const char* Stats_Vert_Shader_Legacy = R"(
**glsl_version**

void main() {
    gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
)";

    constexpr const char* Stats_Frag_Shader_Legacy = R"(
**glsl_version**

uniform vec4 Col;

void main() {
    gl_FragColor = Col;
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

void Statistics::initVBO(bool fixedPipeline) {
    _fixedPipeline = fixedPipeline;

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

    if (ClusterManager::instance()->getMeshImplementation() ==
        ClusterManager::MeshImplementation::BufferObjects)
    {
        if (!_fixedPipeline) {
            glGenVertexArrays(2, &_dynamicVAO[0]);
            glGenVertexArrays(1, &_staticVAO);

            MessageHandler::instance()->printDebug("Statistics: Generating VAOs");
            for (unsigned int i = 0; i < 2; i++) {
                MessageHandler::instance()->printDebug("\t%d", _dynamicVAO[i]);
            }
            MessageHandler::instance()->printDebug("\t%d", _staticVAO);
        }

        glGenBuffers(2, &_dynamicVBO[0]);
        glGenBuffers(1, &_staticVBO);

        MessageHandler::instance()->printDebug("Statistics: Generating VBOs");
        for (unsigned int i = 0; i < 2; i++) {
            MessageHandler::instance()->printDebug("\t%d", _dynamicVBO[i]);
        }
        MessageHandler::instance()->printDebug("\t%d", _staticVBO);
    
        //double buffered VBOs
        for (unsigned int i = 0; i < 2; i++) {
            if (!_fixedPipeline) {
                glBindVertexArray(_dynamicVAO[i]);
                glEnableVertexAttribArray(0);
            }
            glBindBuffer(GL_ARRAY_BUFFER, _dynamicVBO[i]);
            glBufferData(
                GL_ARRAY_BUFFER,
                StatsHistoryLength * sizeof(StatsVertex) * StatsNumberOfDynamicObjs,
                &_dynamicVertexList,
                GL_STREAM_DRAW
            );
    
            if (!_fixedPipeline) {
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
            }
        }

        if (!_fixedPipeline) {
            glBindVertexArray(_staticVAO);
            glEnableVertexAttribArray(0);
        }
        glBindBuffer(GL_ARRAY_BUFFER, _staticVBO);
        glBufferData(
            GL_ARRAY_BUFFER,
            _staticVerts.size() * sizeof(float),
            _staticVerts.data(),
            GL_STATIC_DRAW
        );
        if (!_fixedPipeline) {
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        }
    }
    
    // SETUP SHADERS
    if (_fixedPipeline) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        _shader.setName("StatsShader");
        
        std::string vertShader = Stats_Vert_Shader_Legacy;
        std::string fragShader = Stats_Frag_Shader_Legacy;

        // replace glsl version
        helpers::findAndReplace(
            vertShader,
            "**glsl_version**",
            Engine::instance()->getGLSLVersion()
        );
        helpers::findAndReplace(
            fragShader,
            "**glsl_version**",
            Engine::instance()->getGLSLVersion()
        );
        
        bool vert = _shader.addShaderSrc(
            vertShader,
            GL_VERTEX_SHADER,
            ShaderProgram::ShaderSourceType::String
        );
        if (!vert) {
            MessageHandler::instance()->printError(
                "Failed to load statistics vertex shader"
            );
        }
        bool frag = _shader.addShaderSrc(
            fragShader,
            GL_FRAGMENT_SHADER,
            ShaderProgram::ShaderSourceType::String
        );
        if (!frag) {
            MessageHandler::instance()->printError(
                "Failed to load statistics fragment shader"
            );
        }
        _shader.createAndLinkProgram();
        _shader.bind();

        _colorLoc = _shader.getUniformLocation("Col");

        _shader.unbind();
    }
    else {
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        _shader.setName("StatsShader");

        std::string vertShader = Stats_Vert_Shader;
        std::string fragShader = Stats_Frag_Shader;

        //replace glsl version
        helpers::findAndReplace(
            vertShader,
            "**glsl_version**",
            Engine::instance()->getGLSLVersion()
        );
        helpers::findAndReplace(
            fragShader,
            "**glsl_version**",
            Engine::instance()->getGLSLVersion()
        );

        bool vert = _shader.addShaderSrc(
            vertShader,
            GL_VERTEX_SHADER,
            ShaderProgram::ShaderSourceType::String
        );
        if (!vert) {
            MessageHandler::instance()->printError(
                "Failed to load statistics vertex shader"
            );
        }
        bool frag = _shader.addShaderSrc(
            fragShader,
            GL_FRAGMENT_SHADER,
            ShaderProgram::ShaderSourceType::String
        );
        if (!frag) {
            MessageHandler::instance()->printError(
                "Failed to load statistics fragment shader"
            );
        }
        _shader.createAndLinkProgram();
        _shader.bind();

        _mvpLoc = _shader.getUniformLocation("MVP");
        _colorLoc = _shader.getUniformLocation("Col");

        _shader.unbind();
    }
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
    if (ClusterManager::instance()->getMeshImplementation() !=
        ClusterManager::MeshImplementation::BufferObjects)
    {
        return;
    }

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

    if (_fixedPipeline) {
        // enter ortho mode
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glPushMatrix();
        
        const double size = static_cast<double>(StatsHistoryLength);
        glOrtho(0.0, size, 0.0, size, -1.0, 1.0);

        glMatrixMode(GL_MODELVIEW);

        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glDisable(GL_LIGHTING);

        glLoadIdentity();

        glTranslatef(0.f, static_cast<float>(size) / 4.f, 0.f);
        glScalef(1.f, VertScale, 1.f);

        glLineWidth(lineWidth);

        if (ClusterManager::instance()->getMeshImplementation() ==
            ClusterManager::MeshImplementation::BufferObjects)
        {
            glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
            glEnableClientState(GL_VERTEX_ARRAY);
        
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ARRAY_BUFFER, _staticVBO);
            glVertexPointer(2, GL_FLOAT, 0, reinterpret_cast<void*>(0));

            // draw background (1024x1024 canvas)
            glUniform4fv(_colorLoc, 1, glm::value_ptr(_staticColorBackground));
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            // 1 ms lines
            glUniform4fv(_colorLoc, 1, glm::value_ptr(_staticColorGrid));
            glDrawArrays(GL_LINES, 4, _nLines * 2);

            // zero line, 60hz & 30hz
            glUniform4fv(_colorLoc, 1, glm::value_ptr(_staticColorFrequency));
            glDrawArrays(GL_LINES, 4+_nLines * 2, 6);

            glBindBuffer(GL_ARRAY_BUFFER, _dynamicVBO[_vboIndex]);
            glVertexPointer(2, GL_FLOAT, 0, reinterpret_cast<void*>(0));

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

            // unbind
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glPopClientAttrib();
        }
        else {
            // old-school rendering for OSG compability
            // draw background (1024x1024 canvas)
            glUniform4fv(_colorLoc, 1, glm::value_ptr(_staticColorBackground));
            glBegin(GL_TRIANGLE_STRIP);
            glVertex2f(_staticVerts[0], _staticVerts[1]);
            glVertex2f(_staticVerts[2], _staticVerts[3]);
            glVertex2f(_staticVerts[4], _staticVerts[5]);
            glVertex2f(_staticVerts[6], _staticVerts[7]);
            glEnd();

            // 1 ms lines
            glUniform4fv(_colorLoc, 1, glm::value_ptr(_staticColorGrid));
            glBegin(GL_LINES);
            for (int i = 0; i < _nLines * 4; i += 4) {
                glVertex2f(_staticVerts[i + 8], _staticVerts[i + 9]);
                glVertex2f(_staticVerts[i + 10], _staticVerts[i + 11]);
            }
            glEnd();

            // zero line, 60hz & 30hz
            int offset = _nLines * 4 + 8;
            glUniform4fv(_colorLoc, 1, glm::value_ptr(_staticColorFrequency));
            glBegin(GL_LINES);
            glVertex2f(_staticVerts[offset], _staticVerts[offset + 1]);
            glVertex2f(_staticVerts[offset + 2], _staticVerts[offset + 3]);

            glVertex2f(_staticVerts[offset+4], _staticVerts[offset+5]);
            glVertex2f(_staticVerts[offset+6], _staticVerts[offset+7]);

            glVertex2f(_staticVerts[offset+8], _staticVerts[offset+9]);
            glVertex2f(_staticVerts[offset+10], _staticVerts[offset+11]);
            glEnd();

            // frametime
            glUniform4fv(_colorLoc, 1, glm::value_ptr(_dynamicColors.frameTime));
            glBegin(GL_LINE_STRIP);
            for (unsigned int j = 0; j < StatsHistoryLength; j++) {
                glVertex2f(
                    _dynamicVertexList.frameTime[j].x,
                    _dynamicVertexList.frameTime[j].y
                );
            }
            glEnd();

            // drawtime
            glUniform4fv(_colorLoc, 1, glm::value_ptr(_dynamicColors.drawTime));
            glBegin(GL_LINE_STRIP);
            for (unsigned int j = 0; j < StatsHistoryLength; j++) {
                glVertex2f(
                    _dynamicVertexList.drawTime[j].x,
                    _dynamicVertexList.drawTime[j].y
                );
            }
            glEnd();

            // synctime
            glUniform4fv(_colorLoc, 1, glm::value_ptr(_dynamicColors.syncTime));
            glBegin(GL_LINE_STRIP);
            for (unsigned int j = 0; j < StatsHistoryLength; j++) {
                glVertex2f(
                    _dynamicVertexList.syncTime[j].x,
                    _dynamicVertexList.syncTime[j].y
                );
            }
            glEnd();

            // looptimemax
            glUniform4fv(_colorLoc, 1, glm::value_ptr(_dynamicColors.loopTimeMax));
            glBegin(GL_LINE_STRIP);
            for (unsigned int j = 0; j < StatsHistoryLength; j++) {
                glVertex2f(
                    _dynamicVertexList.loopTimeMax[j].x,
                    _dynamicVertexList.loopTimeMax[j].y
                );
            }
            glEnd();

            // looptimemin
            glUniform4fv(_colorLoc, 1, glm::value_ptr(_dynamicColors.loopTimeMin));
            glBegin(GL_LINE_STRIP);
            for (unsigned int j = 0; j < StatsHistoryLength; j++) {
                glVertex2f(
                    _dynamicVertexList.loopTimeMin[j].x,
                    _dynamicVertexList.loopTimeMin[j].y
                );
            }
            glEnd();
        }

        glPopAttrib();

        // exit ortho mode
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
    }
    else {
        // programmable pipeline
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
    }

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
