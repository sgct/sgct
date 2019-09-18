/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/statistics.h>

#include <GL/glew.h>
#ifdef WIN32
#include <GL/wglew.h>
#elif __LINUX__
#else
#include <OpenGL/glext.h>
#endif
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

namespace sgct_core {

Statistics::Statistics() {
    for (unsigned int i = 0; i < StatsHistoryLength; i++) {
        mDynamicVertexList.frameTime[i].x = static_cast<float>(i);
        mDynamicVertexList.frameTime[i].y = 0.f;
        mDynamicVertexList.drawTime[i].x = static_cast<float>(i);
        mDynamicVertexList.drawTime[i].y = 0.f;
        mDynamicVertexList.syncTime[i].x = static_cast<float>(i);
        mDynamicVertexList.syncTime[i].y = 0.f;
        mDynamicVertexList.loopTimeMax[i].x = static_cast<float>(i);
        mDynamicVertexList.loopTimeMax[i].y = 0.f;
        mDynamicVertexList.loopTimeMin[i].x = static_cast<float>(i);
        mDynamicVertexList.loopTimeMin[i].y = 0.f;
    }
}

Statistics::~Statistics() {
    mShader.deleteProgram();
    
    glDeleteBuffers(2, &mDynamicVBO[0]);
    glDeleteVertexArrays(2, &mDynamicVAO[0]);
    glDeleteBuffers(1, &mStaticVBO);
    glDeleteBuffers(1, &mStaticVAO);
}

void Statistics::initVBO(bool fixedPipeline) {
    mFixedPipeline = fixedPipeline;

    // STATIC BACKGROUND QUAD
    mStaticVerts.push_back(0.f); //x0
    mStaticVerts.push_back(0.f); //y0
    mStaticVerts.push_back(static_cast<float>(StatsHistoryLength * 2)); //x1
    mStaticVerts.push_back(0.f); //y1
    mStaticVerts.push_back(0.f); //x2
    mStaticVerts.push_back(1.f / 30.f); //y2
    mStaticVerts.push_back(static_cast<float>(StatsHistoryLength * 2)); //x3
    mStaticVerts.push_back(1.f / 30.f); //y3;
    
    // STATIC 1 ms LINES
    mNumberOfLines = 0;
    for (float f = 0.001f; f < (1.f / 30.f); f += 0.001f) {
        mStaticVerts.push_back(0.f); //x0
        mStaticVerts.push_back(f); //y0
        mStaticVerts.push_back(static_cast<float>(StatsHistoryLength * 2)); //x1
        mStaticVerts.push_back(f); //y1

        mNumberOfLines++;
    }

    // STATIC 0, 30 & 60 FPS LINES
    mStaticVerts.push_back(0.f); //x0
    mStaticVerts.push_back(0.f); //y0
    mStaticVerts.push_back(static_cast<float>(StatsHistoryLength * 2)); //0x1
    mStaticVerts.push_back(0.f); //y1

    mStaticVerts.push_back(0.f); //x0
    mStaticVerts.push_back(1.f / 30.f); //y0
    mStaticVerts.push_back(static_cast<float>(StatsHistoryLength * 2)); //x1
    mStaticVerts.push_back(1.f / 30.f); //y1

    mStaticVerts.push_back(0.f); //x0
    mStaticVerts.push_back(1.f / 60.f); //y0
    mStaticVerts.push_back(static_cast<float>(StatsHistoryLength * 2)); //x1
    mStaticVerts.push_back(1.f / 60.f); //y1

    if (ClusterManager::instance()->getMeshImplementation() ==
        ClusterManager::MeshImplementation::BufferObjects)
    {
        if (!mFixedPipeline) {
            glGenVertexArrays(2, &mDynamicVAO[0]);
            glGenVertexArrays(1, &mStaticVAO);

            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Debug,
                "Statistics: Generating VAOs:\n"
            );
            for (unsigned int i = 0; i < 2; i++) {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Debug,
                    "\t%d\n", mDynamicVAO[i]
                );
            }
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Debug,
                "\t%d\n\n", mStaticVAO
            );
        }

        glGenBuffers(2, &mDynamicVBO[0]);
        glGenBuffers(1, &mStaticVBO);

        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "Statistics: Generating VBOs:\n"
        );
        for (unsigned int i = 0; i < 2; i++) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Debug,
                "\t%d\n", mDynamicVBO[i]
            );
        }
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "\t%d\n\n", mStaticVBO
        );
    
        //double buffered VBOs
        for (unsigned int i = 0; i < 2; i++) {
            if (!mFixedPipeline) {
                glBindVertexArray(mDynamicVAO[i]);
                glEnableVertexAttribArray(0);
            }
            glBindBuffer(GL_ARRAY_BUFFER, mDynamicVBO[i]);
            glBufferData(
                GL_ARRAY_BUFFER,
                StatsHistoryLength * sizeof(StatsVertex) * StatsNumberOfDynamicObjs,
                &mDynamicVertexList,
                GL_STREAM_DRAW
            );
    
            if (!mFixedPipeline) {
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
            }
        }

        if (!mFixedPipeline) {
            glBindVertexArray(mStaticVAO);
            glEnableVertexAttribArray(0);
        }
        glBindBuffer(GL_ARRAY_BUFFER, mStaticVBO);
        glBufferData(
            GL_ARRAY_BUFFER,
            mStaticVerts.size() * sizeof(float),
            mStaticVerts.data(),
            GL_STATIC_DRAW
        );
        if (!mFixedPipeline) {
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        }
    }
    
    // SETUP SHADERS
    if (mFixedPipeline) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        mShader.setName("StatsShader");
        
        std::string vert_shader = Stats_Vert_Shader_Legacy;
        std::string frag_shader = Stats_Frag_Shader_Legacy;

        //replace glsl version
        sgct_helpers::findAndReplace(
            vert_shader,
            "**glsl_version**",
            sgct::Engine::instance()->getGLSLVersion()
        );
        sgct_helpers::findAndReplace(
            frag_shader,
            "**glsl_version**",
            sgct::Engine::instance()->getGLSLVersion()
        );
        
        bool vert = mShader.addShaderSrc(
            vert_shader,
            GL_VERTEX_SHADER,
            sgct::ShaderProgram::ShaderSourceType::String
        );
        if (!vert) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Failed to load statistics vertex shader\n"
            );
        }
        bool frag = mShader.addShaderSrc(
            frag_shader,
            GL_FRAGMENT_SHADER,
            sgct::ShaderProgram::ShaderSourceType::String
        );
        if (!frag) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Failed to load statistics fragment shader\n"
            );
        }
        mShader.createAndLinkProgram();
        mShader.bind();

        mColLoc = mShader.getUniformLocation("Col");

        mShader.unbind();
    }
    else {
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        mShader.setName("StatsShader");

        std::string vert_shader = Stats_Vert_Shader;
        std::string frag_shader = Stats_Frag_Shader;

        //replace glsl version
        sgct_helpers::findAndReplace(
            vert_shader,
            "**glsl_version**",
            sgct::Engine::instance()->getGLSLVersion()
        );
        sgct_helpers::findAndReplace(
            frag_shader,
            "**glsl_version**",
            sgct::Engine::instance()->getGLSLVersion()
        );

        bool vert = mShader.addShaderSrc(
            vert_shader,
            GL_VERTEX_SHADER,
            sgct::ShaderProgram::ShaderSourceType::String
        );
        if (!vert) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Failed to load statistics vertex shader\n"
            );
        }
        bool frag = mShader.addShaderSrc(
            frag_shader,
            GL_FRAGMENT_SHADER,
            sgct::ShaderProgram::ShaderSourceType::String
        );
        if (!frag) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Failed to load statistics fragment shader\n"
            );
        }
        mShader.createAndLinkProgram();
        mShader.bind();

        mMVPLoc = mShader.getUniformLocation("MVP");
        mColLoc = mShader.getUniformLocation("Col");

        mShader.unbind();
    }
}

void Statistics::setAvgFPS(float afps) {
    mAvgFPS = afps;
}

void Statistics::setFrameTime(float t) {
    mAvgFrameTime = 0.f;
    mMaxFrameTime = 0.f;
    mMinFrameTime = std::numeric_limits<float>::max();

    for (int i = StatsHistoryLength - 2; i >= 0; i--) {
        mDynamicVertexList.frameTime[i + 1].y = mDynamicVertexList.frameTime[i].y;
        if (i < (StatsAverageLength - 1)) {
            mAvgFrameTime += mDynamicVertexList.frameTime[i].y;
            mMaxFrameTime = glm::max(mMaxFrameTime, mDynamicVertexList.frameTime[i].y);
            mMinFrameTime = glm::min(mMinFrameTime, mDynamicVertexList.frameTime[i].y);
        }
    }
    mDynamicVertexList.frameTime[0].y = t;
    
    mAvgFrameTime += mDynamicVertexList.frameTime[0].y;
    mMaxFrameTime = glm::max(mMaxFrameTime, mDynamicVertexList.frameTime[0].y);
    mMinFrameTime = glm::min(mMinFrameTime, mDynamicVertexList.frameTime[0].y);

    mAvgFrameTime /= static_cast<float>(StatsAverageLength);

    float sumSquaredDeviation = 0.f;
    for (int i = 0; i < StatsAverageLength; ++i) {
        sumSquaredDeviation += std::pow(
            mDynamicVertexList.frameTime[i].y - mAvgFrameTime,
            2.f
        );
    }

    mStdDevFrameTime = glm::sqrt(
        sumSquaredDeviation / static_cast<float>(StatsAverageLength)
    );
}

void Statistics::setDrawTime(float t) {
    mAvgDrawTime = 0.f;

    for (int i = StatsHistoryLength - 2; i >= 0; i--) {
        mDynamicVertexList.drawTime[i + 1].y = mDynamicVertexList.drawTime[i].y;
        if (i < (StatsAverageLength - 1)) {
            mAvgDrawTime += mDynamicVertexList.drawTime[i].y;
        }
    }
    mDynamicVertexList.drawTime[0].y = t;
    
    mAvgDrawTime += mDynamicVertexList.drawTime[0].y;
    mAvgDrawTime /= static_cast<float>(StatsAverageLength);
}

void Statistics::setSyncTime(float t) {
    mAvgSyncTime = 0.f;
    
    for (int i = StatsHistoryLength - 2; i >= 0; i--) {
        mDynamicVertexList.syncTime[i + 1].y = mDynamicVertexList.syncTime[i].y;
        if (i < (StatsAverageLength - 1)) {
            mAvgSyncTime += mDynamicVertexList.syncTime[i].y;
        }
    }
    mDynamicVertexList.syncTime[0].y = t;
    
    mAvgSyncTime += mDynamicVertexList.syncTime[0].y;
    mAvgSyncTime /= static_cast<float>(StatsAverageLength);
}

/*!
    Set the minimum and maximum time it takes for a sync message from send to receive
*/
void Statistics::setLoopTime(float min, float max) {
    std::rotate(
        std::rbegin(mDynamicVertexList.loopTimeMax),
        std::rbegin(mDynamicVertexList.loopTimeMax) + 1,
        std::rend(mDynamicVertexList.loopTimeMax)
    );
    mDynamicVertexList.loopTimeMax[0].y = max;
    
    std::rotate(
        std::rbegin(mDynamicVertexList.loopTimeMin),
        std::rbegin(mDynamicVertexList.loopTimeMin) + 1,
        std::rend(mDynamicVertexList.loopTimeMin)
    );
    mDynamicVertexList.loopTimeMin[0].y = min;
}


void Statistics::addSyncTime(float t) {
    mDynamicVertexList.syncTime[StatsHistoryLength - 1].y += t;
    mAvgSyncTime += (t / static_cast<float>(StatsAverageLength));
}

void Statistics::update() {
    if (ClusterManager::instance()->getMeshImplementation() !=
        ClusterManager::MeshImplementation::BufferObjects)
    {
        return;
    }

    mVBOIndex = 1 - mVBOIndex; //ping-pong
    glBindBuffer(GL_ARRAY_BUFFER, mDynamicVBO[mVBOIndex]);
    size_t size = StatsHistoryLength * sizeof(StatsVertex) * StatsNumberOfDynamicObjs;

    //glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STREAM_DRAW);
    
    GLvoid* positionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    
    if (positionBuffer) {
        memcpy(positionBuffer, &mDynamicVertexList, size);
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Statistics::draw(float lineWidth) {
    mShader.bind();

    if (mFixedPipeline) {
        //enter ortho mode
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glPushMatrix();
        
        double size = static_cast<double>(StatsHistoryLength);
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
            glBindBuffer(GL_ARRAY_BUFFER, mStaticVBO);
            glVertexPointer(2, GL_FLOAT, 0, reinterpret_cast<void*>(0));

            //draw background (1024x1024 canvas)
            glUniform4fv(mColLoc, 1, glm::value_ptr(mStaticColorBackground));
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            //1 ms lines
            glUniform4fv(mColLoc, 1, glm::value_ptr(mStaticColorGrid));
            glDrawArrays(GL_LINES, 4, mNumberOfLines * 2);

            //zero line, 60hz & 30hz
            glUniform4fv(mColLoc, 1, glm::value_ptr(mStaticColorFrequency));
            glDrawArrays(GL_LINES, 4+mNumberOfLines * 2, 6);

            glBindBuffer(GL_ARRAY_BUFFER, mDynamicVBO[mVBOIndex]);
            glVertexPointer(2, GL_FLOAT, 0, reinterpret_cast<void*>(0));

            // frametime
            glUniform4fv(mColLoc, 1, glm::value_ptr(mDynamicColors.frameTime));
            glDrawArrays(GL_LINE_STRIP, 0 * StatsHistoryLength, StatsHistoryLength);
            // drawtime
            glUniform4fv(mColLoc, 1, glm::value_ptr(mDynamicColors.drawTime));
            glDrawArrays(GL_LINE_STRIP, 1 * StatsHistoryLength, StatsHistoryLength);
            // synctime
            glUniform4fv(mColLoc, 1, glm::value_ptr(mDynamicColors.syncTime));
            glDrawArrays(GL_LINE_STRIP, 2 * StatsHistoryLength, StatsHistoryLength);
            // looptimemax
            glUniform4fv(mColLoc, 1, glm::value_ptr(mDynamicColors.loopTimeMax));
            glDrawArrays(GL_LINE_STRIP, 3 * StatsHistoryLength, StatsHistoryLength);
            // looptimemin
            glUniform4fv(mColLoc, 1, glm::value_ptr(mDynamicColors.loopTimeMin));
            glDrawArrays(GL_LINE_STRIP, 4 * StatsHistoryLength, StatsHistoryLength);

            //unbind
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glPopClientAttrib();
        }
        else {
            //old-school rendering for OSG compability
            //draw background (1024x1024 canvas)
            glUniform4fv(mColLoc, 1, glm::value_ptr(mStaticColorBackground));
            glBegin(GL_TRIANGLE_STRIP);
            glVertex2f(mStaticVerts[0], mStaticVerts[1]);
            glVertex2f(mStaticVerts[2], mStaticVerts[3]);
            glVertex2f(mStaticVerts[4], mStaticVerts[5]);
            glVertex2f(mStaticVerts[6], mStaticVerts[7]);
            glEnd();

            //1 ms lines
            glUniform4fv(mColLoc, 1, glm::value_ptr(mStaticColorGrid));
            glBegin(GL_LINES);
            for (int i = 0; i < mNumberOfLines * 4; i += 4) {
                glVertex2f(mStaticVerts[i + 8], mStaticVerts[i + 9]);
                glVertex2f(mStaticVerts[i + 10], mStaticVerts[i + 11]);
            }
            glEnd();

            //zero line, 60hz & 30hz
            int offset = mNumberOfLines * 4 + 8;
            glUniform4fv(mColLoc, 1, glm::value_ptr(mStaticColorFrequency));
            glBegin(GL_LINES);
            glVertex2f(mStaticVerts[offset], mStaticVerts[offset + 1]);
            glVertex2f(mStaticVerts[offset + 2], mStaticVerts[offset + 3]);

            glVertex2f(mStaticVerts[offset+4], mStaticVerts[offset+5]);
            glVertex2f(mStaticVerts[offset+6], mStaticVerts[offset+7]);

            glVertex2f(mStaticVerts[offset+8], mStaticVerts[offset+9]);
            glVertex2f(mStaticVerts[offset+10], mStaticVerts[offset+11]);
            glEnd();

            // frametime
            glUniform4fv(mColLoc, 1, glm::value_ptr(mDynamicColors.frameTime));
            glBegin(GL_LINE_STRIP);
            for (unsigned int j = 0; j < StatsHistoryLength; j++) {
                glVertex2f(
                    mDynamicVertexList.frameTime[j].x,
                    mDynamicVertexList.frameTime[j].y
                );
            }
            glEnd();

            // drawtime
            glUniform4fv(mColLoc, 1, glm::value_ptr(mDynamicColors.drawTime));
            glBegin(GL_LINE_STRIP);
            for (unsigned int j = 0; j < StatsHistoryLength; j++) {
                glVertex2f(
                    mDynamicVertexList.drawTime[j].x,
                    mDynamicVertexList.drawTime[j].y
                );
            }
            glEnd();

            // synctime
            glUniform4fv(mColLoc, 1, glm::value_ptr(mDynamicColors.syncTime));
            glBegin(GL_LINE_STRIP);
            for (unsigned int j = 0; j < StatsHistoryLength; j++) {
                glVertex2f(
                    mDynamicVertexList.syncTime[j].x,
                    mDynamicVertexList.syncTime[j].y
                );
            }
            glEnd();

            // looptimemax
            glUniform4fv(mColLoc, 1, glm::value_ptr(mDynamicColors.loopTimeMax));
            glBegin(GL_LINE_STRIP);
            for (unsigned int j = 0; j < StatsHistoryLength; j++) {
                glVertex2f(
                    mDynamicVertexList.loopTimeMax[j].x,
                    mDynamicVertexList.loopTimeMax[j].y
                );
            }
            glEnd();

            // looptimemin
            glUniform4fv(mColLoc, 1, glm::value_ptr(mDynamicColors.loopTimeMin));
            glBegin(GL_LINE_STRIP);
            for (unsigned int j = 0; j < StatsHistoryLength; j++) {
                glVertex2f(
                    mDynamicVertexList.loopTimeMin[j].x,
                    mDynamicVertexList.loopTimeMin[j].y
                );
            }
            glEnd();
        }

        glPopAttrib();

        //exit ortho mode
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
        
        glUniformMatrix4fv(mMVPLoc, 1, GL_FALSE, &orthoMat[0][0]);
        
        glBindVertexArray(mStaticVAO);

        //draw background (1024x1024 canvas)
        glUniform4fv(mColLoc, 1, glm::value_ptr(mStaticColorBackground));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        //1 ms lines
        glUniform4fv(mColLoc, 1, glm::value_ptr(mStaticColorGrid));
        glDrawArrays(GL_LINES, 4, mNumberOfLines * 2);
        
        //zero line, 60hz & 30hz
        glUniform4fv(mColLoc, 1, glm::value_ptr(mStaticColorFrequency));
        glDrawArrays(GL_LINES, 4 + mNumberOfLines * 2, 6);
        
        glBindVertexArray(mDynamicVAO[mVBOIndex]);

        // frametime
        glUniform4fv(mColLoc, 1, glm::value_ptr(mDynamicColors.frameTime));
        glDrawArrays(GL_LINE_STRIP, 0 * StatsHistoryLength, StatsHistoryLength);

        // drawtime
        glUniform4fv(mColLoc, 1, glm::value_ptr(mDynamicColors.drawTime));
        glDrawArrays(GL_LINE_STRIP, 1 * StatsHistoryLength, StatsHistoryLength);

        // synctime
        glUniform4fv(mColLoc, 1, glm::value_ptr(mDynamicColors.syncTime));
        glDrawArrays(GL_LINE_STRIP, 2 * StatsHistoryLength, StatsHistoryLength);

        // looptimemax
        glUniform4fv(mColLoc, 1, glm::value_ptr(mDynamicColors.loopTimeMax));
        glDrawArrays(GL_LINE_STRIP, 3 * StatsHistoryLength, StatsHistoryLength);

        // looptimemin
        glUniform4fv(mColLoc, 1, glm::value_ptr(mDynamicColors.loopTimeMin));
        glDrawArrays(GL_LINE_STRIP, 4 * StatsHistoryLength, StatsHistoryLength);

        //unbind
        glBindVertexArray(0);
    }

    mShader.unbind();
}

float Statistics::getAvgFPS() const {
    return mAvgFPS;
}

float Statistics::getAvgDrawTime() const {
    return mAvgDrawTime;
}

float Statistics::getAvgSyncTime() const {
    return mAvgSyncTime;
}

float Statistics::getAvgFrameTime() const {
    return mAvgFrameTime;
}

float Statistics::getMinFrameTime() const {
    return mMinFrameTime;
}

float Statistics::getMaxFrameTime() const {
    return mMaxFrameTime;
}

float Statistics::getFrameTimeStandardDeviation() const {
    return mStdDevFrameTime;
}

float Statistics::getFrameTime() const {
    return mDynamicVertexList.frameTime[0].y;
}

float Statistics::getDrawTime() const {
    return mDynamicVertexList.drawTime[0].y;
}

float Statistics::getSyncTime() const {
    return mDynamicVertexList.syncTime[0].y;
}

} // namespace sgct_core
